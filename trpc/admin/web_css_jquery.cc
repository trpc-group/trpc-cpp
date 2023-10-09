//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#include "trpc/admin/web_css_jquery.h"

#include <utility>

namespace trpc::admin {

const char* HtmlHeader() {
  return "<!DOCTYPE html>\n<html>\n"
         "<head>\n"
         "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"\n>"
         "<script language=\"javascript\" type=\"text/javascript\" "
         "src=\"/cmdsweb/js/jquery_min\"></script>\n";
}

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/0.9.6-rc02/src/brpc/builtin/common.cpp.
// https://github.com/apache/brpc/blob/0.9.6-rc02/src/brpc/builtin/vars_service.cpp.

const char* TabsHead() {
    return
        "<style type=\"text/css\">\n"
        "ol,ul { list-style:none; }\n"
        ".tabs-menu {\n"
        "    position: fixed;"
        "    top: 0px;"
        "    left: 0px;"
        "    height: 40px;\n"
        "    width: 100%;\n"
        "    clear: both;\n"
        "    padding: 0px;\n"
        "    margin: 0px;\n"
        "    background-color: blue;\n"
        "    border:none;\n"
        "    overflow: hidden;\n"
        "    box-shadow: 0px 1px 2px #909090;\n"
        "    z-index: 5;\n"
        "}\n"
        ".tabs-menu li {\n"
        "    float:left;\n"
        "    fill:none;\n"
        "    border:none;\n"
        "    padding:10px 30px 10px 30px;\n"
        "    text-align:center;\n"
        "    cursor:pointer;\n"
        "    color:#dddddd;\n"
        "    font-weight: bold;\n"
        "    font-family: \"Segoe UI\", Calibri, Arial;\n"
        "}\n"
        ".tabs-menu li.current {\n"
        "    color:#FFFFFF;\n"
        "    background-color: #303030;\n"
        "}\n"
        ".tabs-menu li.help {\n"
        "    float:right;\n"
        "}\n"
        ".tabs-menu li:hover {\n"
        "    background-color: #303030;\n"
        "}\n"
        "</style>\n"
        "<script type=\"text/javascript\">\n"
        "$(function() {\n"
        "  $(\".tabs-menu li\").click(function(event) {\n"
        "    window.location.href = $(this).attr('id');\n"
        "  });\n"
        "});\n"
        "</script>\n";
}

const char* GridTableStyle() {
    return
        "<style type=\"text/css\">\n"
        "table.gridtable {\n"
        "  color:#333333;\n"
        "  border-width:1px;\n"
        "  border-color:#666666;\n"
        "  border-collapse:collapse;\n"
        "}\n"
        "table.gridtable th {\n"
        "  border-width:1px;\n"
        "  padding:3px;\n"
        "  border-style:solid;\n"
        "  border-color:#666666;\n"
        "  background-color:#eeeeee;\n"
        "}\n"
        "table.gridtable td {\n"
        "  border-width:1px;\n"
        "  padding:3px;\n"
        "  border-style:solid;\n"
        "  border-color:#666666;\n"
        "  background-color:#ffffff;\n"
        "}\n"
        "</style>\n";
}


const char* PutSysvarsHeading() {
  return
        "<style type=\"text/css\">\n"
        "#layer1 { margin:0; padding:0; width:1111px; }\n"
        // style of plot-able tvar
        ".variable {\n"
        "  margin:0px;\n"
        "  color:#000000;\n"
        "  cursor:pointer;\n"
        "  position:relative;\n"
        "  background-color:#ffffff;\n"
        "}\n"
        // style of non-plot-able tvar, the difference with .variable is only
        // the cursor .
        ".nonplot-variable {\n"
        "  margin:0px;\n"
        "  color:#000000;\n"
        "  position:relative;\n"
        "  background-color:#ffffff;\n"
        "}\n"
        // style of <p>
        "p {padding: 2px 0; margin: 0px; }\n"
        // style of container of flot graph.
        ".detail {\n"
        "  margin: 0px;\n"
        "  width: 800px;\n"
        "  background-color:#fafafa;\n"
        "}\n"
        ".flot-placeholder {\n"
        "  width: 800px;\n"
        "  height: 200px;\n"
        "  line-height: 1.2em;\n"
        "}\n"
        "</style>\n"

        "<script type=\"text/javascript\">\n"
        // Mark if a tvar was ever clicked.
        "var everEnabled = {}\n"
        // Mark if a tvar was enabled ploting
        "var enabled = {}\n"
        // the tvar under cursor
        "var hovering_var = \"\"\n"
        // timeout id of last server call.
        "var timeoutId = {}\n"
        // last plot of the tvar.
        "var lastPlot = {}\n"

        "function prepareGraphs() {\n"
        // Hide all graphs at first.
        "  $(\".detail\").hide();\n"

        // Register clicking functions.
        "  $(\".variable\").click(function() {\n"
        "    var mod = $(this).next(\".detail\");\n"
        "    mod.slideToggle(\"fast\");\n"
        "    var var_name = mod.children(\":first-child\").attr(\"id\");\n"
        "    if (!everEnabled[var_name]) {\n"
        "      everEnabled[var_name] = true;\n"
        // Create tooltip at first click.
        "      $(\"<div id='tooltip-\" + var_name + \"'></div>\").css({\n"
        "        position: \"absolute\",\n"
        "        display: \"none\",\n"
        "        border: \"1px solid #fdd\",\n"
        "        padding: \"2px\",\n"
        "        \"background-color\": \"#ffffca\",\n"
        "        opacity: 0.80\n"
        "      }).appendTo(\"body\");\n"
        // Register hovering event and show the tooltip when event occurs.
        "      $(\"#\" + var_name).bind(\"plothover\", function(event, pos, item) {\n"
        "        if (item) {\n"
        "          hovering_var = var_name;\n"
        "          var thePlot = lastPlot[var_name];\n"
        "          if (thePlot != null) {\n"
        "            item.series.color = \"#808080\";\n"
        "            thePlot.draw();\n"
        "          }\n"
        "          var x = item.datapoint[0];\n"
        "          var y = item.datapoint[1];\n"
        "          $(\"#tooltip-\" + var_name)\n"
        "            .html(y + \"<br/>(\" + describeX(x, item.series) + \")\")\n"
        "            .css({top: item.pageY+5, left: item.pageX+15})\n"
        "            .show();\n"
        "        } else {\n"
        "            hovering_var = \"\";\n"
        "            $(\"#tooltip-\" + var_name).hide();\n"
        "        }\n"
        "      });\n"
        // Register mouseleave to make sure the tooltip is hidden when cursor
        // is out.
        "      $(\"#\" + var_name).bind(\"mouseleave\", function() {\n"
        "        $(\"#tooltip-\" + var_name).hide();\n"
        "      });\n"
        "    }\n"
        "    if (!enabled[var_name]) {\n"
        "      enabled[var_name] = true;\n"
        "      fetchData(var_name);\n"
        "    } else {\n"
        "      enabled[var_name] = false;\n"
        "      clearTimeout(timeoutId[var_name]);\n"
        "    }\n"
        "  });\n"
        "  $(\".default_expand\").click();\n"
        "}\n"
        // options for ploting.
        "var trendOptions = {\n"
        "  colors: ['#F0D06E','#F0B06E','#F0A06E','#F0906E','#F0806E'],\n"
        "  legend: {show:false},\n"
        "  grid: {hoverable:true},\n"
        "  xaxis: { \n"
        "    \"ticks\": [[29,\"-1 day\"],[53,\"-1 hour\"],[113,\"-1 minute\"]]\n"
        "  }\n"
        "}\n"
        "var cdfOptions = {\n"
        "  grid: {hoverable: true},\n"
        "  lines: {\n"
        "    show: true,\n"
        "    fill: true\n"
        "  },\n"
        "  xaxis: {\n"
        "    \"ticks\": [[10,\"10%\"],[20,\"20%\"],[30,\"30%\"],[40,\"40%\"]\n"
        "               ,[50,\"50%\"],[60,\"60%\"],[70,\"70%\"],[80,\"80%\"]\n"
        "               ,[90,\"90%\"],[101,\"99.99%\"]]\n"
        "  }\n"
        "}\n"

        // Show x in tooltip intuitively.
        "function describeTrendX(x) {\n"
        "  if (x >= 173) {\n"
        "    return \"just now\";\n"
        "  } else if (x > 113) {\n"
        "    return (x - 173) + \" second\";\n"
        "  } else if (x > 53) {\n"
        "    return (x - 114) + \" minute\";\n"
        "  } else if (x > 29) {\n"
        "    return (x - 54) + \" hour\";\n"
        "  } else {\n"
        "    return (x - 30) + \" day\";\n"
        "  }\n"
        "}\n"
        "function describeCDFX(x) {\n"
        "  if (x <= 99) {\n"
        "    return x + '%';\n"
        "  } else if (x == 100) {\n"
        "    return '99.9%';\n"
        "  } else if (x == 101) {\n"
        "    return '99.99%';\n"
        "  } else {\n"
        "    return 'unknown ' + x;\n"
        "  }\n"
        "}\n"
        "function describeX(x, series) {\n"
        "  if (series.data[series.data.length-1][0] == 173) {\n"
        "    if (series.label != null) {\n"
        "      return series.label + ' ' + describeTrendX(x);\n"
        "    } else {\n"
        "      return describeTrendX(x);\n"
        "    }\n"
        "  } else if (series.data[series.data.length-1][0] == 101) {\n"
        "    return describeCDFX(x);\n"
        "  } else {\n"
        "    return x;\n"
        "  }\n"
        "}\n"
        // Get value series of tvar from server.
        "function fetchData(var_name) {\n"
        "  function onDataReceived(series) {\n"
        "    if (hovering_var != var_name) {\n"
        "      if (series.label == 'trend') {\n"
        "        lastPlot[var_name] = $.plot(\"#\" + var_name, [series.data], trendOptions);\n"
        "        $(\"#value-\" + var_name).html(series.data[series.data.length - 1][1]);\n"
        "      } else if (series.label == 'cdf') {\n"
        "        lastPlot[var_name] = $.plot(\"#\" + var_name, [series.data], cdfOptions);\n"
        "        $(\"#value-\" + var_name).html(series.data[series.data.length - 1][1]);\n"
        "      } else {\n"
        "        lastPlot[var_name] = $.plot(\"#\" + var_name, series, trendOptions);\n"
        "        var newValue = '\"[';\n"
        "        var i;\n"
        "        for (i = 0; i < series.length; ++i) {\n"
        "            if (i != 0) newValue += ',';\n"
        "            var data = series[i].data;\n"
        "            newValue += data[data.length - 1][1];\n"
        "        }\n"
        "        newValue += ']\"';\n"
        "        $(\"#value-\" + var_name).html(newValue);\n"
        "      }\n"
        "    }\n"
        "  }\n"
        "  $.ajax({\n"
        "    url: \"/cmdsweb/seriesvar?var=\" + var_name,\n"
        "    type: \"GET\",\n"
        "    dataType: \"json\",\n"
        "    success: onDataReceived\n"
        "  });\n"
        "  if (enabled[var_name]) {\n"
        "    timeoutId[var_name] = setTimeout(function(){ fetchData(var_name); }, 1000);\n"
        "  }\n"
        "}\n"
        "$(prepareGraphs);\n"
        "</script>\n";
}

// End of source codes that are from incubator-brpc.

void SetTabItems(std::map<std::string, CmdInfo> *m_tabs) {
  CmdInfo cmd_info;
  cmd_info.path = "/cmdsweb/stats";
  cmd_info.desc = "show the process stats info";
  m_tabs->insert(std::make_pair("stats", cmd_info));

  cmd_info.path = "/cmdsweb/logs";
  cmd_info.desc = "show the log config and set log level";
  m_tabs->insert(std::make_pair("logs", cmd_info));

  cmd_info.path = "/cmdsweb/profile/cpu";
  cmd_info.desc = "show the process hot functions";
  m_tabs->insert(std::make_pair("cpu", cmd_info));

  cmd_info.path = "/cmdsweb/profile/heap";
  cmd_info.desc = "show the process heap memory alloction";
  m_tabs->insert(std::make_pair("heap", cmd_info));

  cmd_info.path = "/cmdsweb/config";
  cmd_info.desc = "show the config";
  m_tabs->insert(std::make_pair("config", cmd_info));

  /*
  cmd_info.path = "/cmdsweb/profile/contention";
  cmd_info.desc = "show the process lock contention";
  m_tabs->insert(std::make_pair("contention", cmd_info));
  */

  cmd_info.path = "/cmdsweb/sysvars";
  cmd_info.desc = "show the system variable info";
  m_tabs->insert(std::make_pair("sysvars", cmd_info));
}


void PrintTabsBody(std::string *html,
                   const std::map<std::string, CmdInfo>& m_tabs) {
  html->append("<ul class='tabs-menu'>\n");
  for (auto it : m_tabs) {
    html->append("<li id='");
    html->append(it.second.path);
    html->append("'>");
    html->append(it.first);
    html->append("</li>\n");
  }
  html->append("</ul>\n");
}


void PrintDisplayInfo(std::string *html, const std::string& dtype) {
  html->append("<div>\n<pre style='display:inline'>type:&nbsp&nbsp</pre>\n");
  html->append("<select id='display_type' onchange='onSetDisplayType()'>\n");
  html->append("<option value='dot'");
  if (strncmp(dtype.c_str(), kDisplayDot, 3) == 0) {
    html->append(" selected");
  }
  html->append(">dot</option>\n");

  html->append("<option value='text'");
  if (dtype == kDisplayText) {
    html->append(" selected");
  }
  html->append(">text</option>\n");

  html->append("</select>\n</div>\n");
}


void PrintViewListInfo(std::string *html, const std::string& vname,
                      const std::map<std::string, std::string> &mfiles) {
  html->append("<div>\n<pre style='display:inline'>View:&nbsp&nbsp</pre>\n");
  html->append("<select id='view_name' onchange='onSetViewName()'>\n");
  html->append("<option value='new_profile'>new_profile</option>\n");

  for (auto &ff : mfiles) {
    std::string fname = ff.first;
    html->append("<option value='");
    html->append(fname);
    if (vname == fname) {
      html->append("' selected>");
    } else {
      html->append("'>");
    }
    html->append(ff.second);
    html->append("</option>\n");
  }
  html->append("</select>\n</div>\n");
}


const char* CommonOnRecvData() {
  return
  "<script type=\"text/javascript\">\n"
  "  function onDataReceived(data) {\n"
  "    var h_seconds = document.getElementById('hidden_seconds').value;\n"
  "    var h_dtype = document.getElementById('hidden_display_type').value;\n"
  "    var h_vname = document.getElementById('hidden_view_name').value;\n"
  "    var selEnd = data.indexOf('[delViewName]');\n"
  "    if (selEnd != -1) {\n"
  "      var del_val = data.substring(0, selEnd);\n"
  "      var mySelect = document.getElementById('view_name');\n"
  "      for(var i=0; i < mySelect.length; i++) {\n"
  "        if(mySelect.options[i].value == del_val) {\n"
  "          mySelect.options.remove(i);\n"
  "          break;\n"
  "        }\n"
  "      }\n"
  "      data = data.substring(selEnd + '[delViewName]'.length);\n"
  "    }\n"
  "    selEnd = data.indexOf('[addViewName]');\n"
  "    if (selEnd != -1) {\n"
  "      var option = document.createElement('option');\n"
  "      option.value = data.substring(0, selEnd);\n"
  "      option.text = option.value;\n"
  "      h_vname = option.value;\n"
  "      var slash_index = option.value.lastIndexOf('/');\n"
  "      if (slash_index != -1) {\n"
  "        option.text = option.value.substring(slash_index + 1);\n"
  "      }\n"
  "      var sel = document.getElementById('view_name');\n"
  "      var option1 = sel.options[1];\n"
  "      if (option1 == null || option1.text != option.text) {\n"
  "        sel.add(option, 1);\n"
  "      } else if (option1 != null) {\n"
  "        console.log('merged ' + option.text);\n"
  "      }\n"
  "      sel.selectedIndex = 1;\n";
}

}  // namespace trpc::admin
