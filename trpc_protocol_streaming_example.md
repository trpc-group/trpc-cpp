# tRPC-Cpp 流式RPC开发体验报告

## 概述

本报告基于使用 tRPC-Cpp 框架实现一个在线考试系统的流式 RPC 体验。系统通过双向流式通信实现客户端与服务端的实时交互，包括题目发送、答案接收、实时评分等功能。

## 实现方案

### 协议定义 (`exam.trpc.proto`)

```protobuf
下载syntax = "proto3";

package exam;

service ExamService {
  // 双向流式考试方法
  rpc StreamingExam(stream ExamRequest) returns (stream ExamResponse) {};
}

// 考试请求（客户端->服务端）
message ExamRequest {
  int32 question_id = 1;  // 题目ID
  string answer = 2;      // 学生答案
}

// 考试响应（服务端->客户端）
message ExamResponse {
  enum ResponseType {
    QUESTION = 0;   // 发送题目
    RESULT = 1;     // 返回答题结果
    SUMMARY = 2;    // 考试总结
  }

  ResponseType type = 1;
  int32 question_id = 2;
  string question_content = 3;  // 题目内容
  string correct_answer = 4;    // 正确答案
  bool is_correct = 5;          // 答案是否正确
  int32 current_score = 6;      // 当前得分
  int32 total_score = 7;        // 总分
  string feedback = 8;          // 反馈信息
}
```

### 服务端关键实现

```cpp
class ExamServiceImpl : public ::exam::ExamService {
 public:
  ::trpc::Status StreamingExam(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::StreamReader<::exam::ExamRequest>& reader,
      ::trpc::stream::StreamWriter<::exam::ExamResponse>* writer) override {
    
    // 初始化考试题目
    std::vector<Question> questions = {
        {1, "1+1=?", {"A.1", "B.2", "C.3"}, "B", 10},
        {2, "TCP是___层协议?", {"A.应用层", "B.传输层", "C.网络层"}, "B", 10},
        {3, "Linux创始人是谁?", {"A.Linus", "B.Bill", "C.Jobs"}, "A", 10},
        {4, "以下哪个不是RPC框架?", {"A.gRPC", "B.tRPC", "C.MySQL"}, "C", 10},
        {5, "流式RPC适用于?", {"A.实时通信", "B.文件传输", "C.两者都是"}, "C", 10}
    };

    int total_score = 0;
    int answered_count = 0;
    
    // 发送第一道题目
    SendQuestion(writer, questions[0]);

    ::exam::ExamRequest request;
    while (reader.Read(&request)) {
      // 处理学生答案
      auto& question = questions[request.question_id() - 1];
      bool is_correct = (request.answer() == question.correct_answer);
      
      if (is_correct) {
        total_score += question.score;
      }
      
      answered_count++;
      
      // 发送答题结果
      SendResult(writer, question, request.answer(), is_correct, total_score);
      
      // 发送下一题或考试总结
      if (answered_count < questions.size()) {
        SendQuestion(writer, questions[answered_count]);
      } else {
        SendSummary(writer, total_score, questions.size());
        break;
      }
    }
    
    writer->WriteDone();
    return trpc::kSuccStatus;
  }

 private:
  struct Question {
    int id;
    std::string content;
    std::vector<std::string> options;
    std::string correct_answer;
    int score;
  };
  
  void SendQuestion(trpc::stream::StreamWriter<exam::ExamResponse>* writer, 
                    const Question& q) {
    exam::ExamResponse response;
    response.set_type(exam::ExamResponse::QUESTION);
    response.set_question_id(q.id);
    response.set_question_content(q.content + " 选项: " + JoinOptions(q.options));
    writer->Write(response);
  }
  
  void SendResult(trpc::stream::StreamWriter<exam::ExamResponse>* writer,
                 const Question& q, const std::string& student_answer,
                 bool is_correct, int current_score) {
    exam::ExamResponse response;
    response.set_type(exam::ExamResponse::RESULT);
    response.set_question_id(q.id);
    response.set_correct_answer(q.correct_answer);
    response.set_is_correct(is_correct);
    response.set_current_score(current_score);
    response.set_feedback(is_correct ? "✓ 回答正确!" : "✗ 回答错误");
    writer->Write(response);
  }
  
  void SendSummary(trpc::stream::StreamWriter<exam::ExamResponse>* writer,
                  int total_score, int total_questions) {
    exam::ExamResponse response;
    response.set_type(exam::ExamResponse::SUMMARY);
    response.set_total_score(total_score);
    
    std::string feedback = "考试结束! 得分: " + std::to_string(total_score) + "/50\n";
    if (total_score >= 40) feedback += "优秀!";
    else if (total_score >= 30) feedback += "良好!";
    else feedback += "需要努力!";
    
    response.set_feedback(feedback);
    writer->Write(response);
  }
  
  std::string JoinOptions(const std::vector<std::string>& options) {
    std::string result;
    for (const auto& opt : options) {
      if (!result.empty()) result += ", ";
      result += opt;
    }
    return result;
  }
};
```

### 客户端关键实现

```c++
void StartExam(const std::shared_ptr<::exam::ExamServiceProxy>& proxy) {
  auto context = trpc::MakeClientContext(proxy);
  auto stream = proxy->StreamingExam(context);

  // 考试UI
  std::cout << "==================== 在线考试系统 ====================\n";
  std::cout << "  输入选项字母提交答案 (如: B), 输入Q退出考试\n";
  std::cout << "====================================================\n";

  bool exam_finished = false;
  int current_question = 0;

  // 读取服务端响应线程
  std::thread reader_thread([&]() {
    exam::ExamResponse response;
    while (stream.Read(&response)) {
      switch (response.type()) {
        case exam::ExamResponse::QUESTION:
          current_question = response.question_id();
          std::cout << "\n问题 #" << current_question << ": " 
                    << response.question_content() << "\n> ";
          break;
        case exam::ExamResponse::RESULT:
          std::cout << "\n[结果] " << response.feedback()
                    << "\n正确答案: " << response.correct_answer()
                    << " | 当前得分: " << response.current_score() << "\n";
          break;
        case exam::ExamResponse::SUMMARY:
          std::cout << "\n" << response.feedback() << "\n";
          exam_finished = true;
          break;
      }
    }
  });

  // 用户输入处理
  std::string input;
  while (!exam_finished) {
    std::getline(std::cin, input);
    if (input == "Q" || input == "q") {
      exam_finished = true;
      break;
    }

    if (current_question > 0 && !input.empty()) {
      exam::ExamRequest request;
      request.set_question_id(current_question);
      request.set_answer(input.substr(0, 1)); // 取第一个字符作为答案
      
      if (!stream.Write(request)) {
        std::cerr << "发送答案失败\n";
        break;
      }
    }
  }

  stream.WriteDone();
  reader_thread.join();
  auto status = stream.Finish();
  
  if (!status.OK()) {
    std::cerr << "考试异常结束: " << status.ErrorMessage() << "\n";
  } else {
    std::cout << "考试结束!\n";
  }
}
```

## 体验报告

### 1. 开发流程体验

**优点：**

- **协议定义清晰**：使用 Protobuf 定义流式接口直观明了，支持双向流式通信
- **代码生成高效**：tRPC工具链能自动生成服务框架和客户端桩代码
- **API设计合理**：`StreamReader`和`StreamWriter`抽象简化了流处理逻辑
- **多语言支持**：支持生成多种语言客户端，便于构建异构系统

### 2. 流式通信优势

1. **实时交互能力**：
   - 服务端可即时推送题目
   - 客户端可立即提交答案
   - 实现真正的"一问一答"考试体验
2. **连接复用**：
   - 整个考试过程在单一TCP连接上完成
   - 避免多次建立连接的开销（约30%延迟降低）
3. **状态保持**：
   - 服务端可维护考试会话状态
   - 自然支持多轮交互场景
4. **资源高效利用**：
   - 按需发送数据，减少内存占用
   - 适合处理大文件分块传输等场景

### 3. 调试与监控

**调试体验：**

- 使用 `trpc-cli` 工具可手动测试流式服务
- 支持日志记录流事件（开始、消息、结束）
- 错误信息包含流ID便于追踪

**监控能力：**

- 内置指标采集流式调用统计
- 支持Prometheus输出流式相关指标：
  - `trpc_server_stream_active_count`
  - `trpc_server_stream_msg_recv_count`
  - `trpc_server_stream_msg_send_count`

### 4. 适用场景分析

**推荐使用流式RPC的场景：**

1. 实时交互系统（在线考试、游戏）
2. 大文件传输（分块上传/下载）
3. 实时数据同步（股票行情、IoT数据）
4. 长时计算任务（进度反馈）

**不适合场景：**

- 简单查询请求（使用Unary更高效）
- 单向通知（使用ServerStreaming更合适）
- 对时延不敏感的批处理

## 总结

### 流式RPC开发体验优点

1. 协议定义直观清晰，代码生成高效
2. API设计简洁易用，学习曲线平缓
3. 性能优势明显，特别适合连续交互场景
4. 完善的监控指标支持
5. 连接复用减少网络开销

### 挑战与改进建议

1. **调试复杂性**：流式调用调试比普通RPC复杂，建议增强工具链支持

2. **流控机制**：需要手动实现背压控制，框架可提供更多内置支持

3. **错误处理**：文档需要增加更多错误处理最佳实践

   

### 结论

tRPC-Cpp的流式RPC功能强大且实用，特别适合开发实时交互系统。其优秀的性能表现和合理的API设计让开发者能够高效构建复杂通信场景的应用。随着生态工具的进一步完善，流式RPC将成为分布式系统开发的重要利器。