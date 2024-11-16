do
    local protocol_name    = "trpc" 
    local trpc_proto       = Proto(protocol_name, "tRPC Protocol Dissector")
    
    local field_magic      = ProtoField.uint16(protocol_name .. ".magic",       "Magic",       base.HEX)
    local field_type       = ProtoField.uint8(protocol_name .. ".type",         "Packet Type", base.DEC)
    local field_stream     = ProtoField.uint8(protocol_name .. ".stream",       "Stream Type", base.DEC)
    local field_total_size = ProtoField.uint32(protocol_name .. ".total_size",  "Total Size",  base.DEC)
    local field_header_size= ProtoField.uint16(protocol_name .. ".header_size", "Header Size", base.DEC)
    local field_unique_id  = ProtoField.uint32(protocol_name .. ".unique_id",   "Unique ID",   base.DEC)
    local field_version    = ProtoField.uint8(protocol_name .. ".version",      "Version",     base.DEC)
    local field_reserved   = ProtoField.uint8(protocol_name .. ".reserved",     "Reserved",    base.DEC)
    trpc_proto.fields      = {field_magic, field_type, field_stream, field_total_size, field_header_size, field_unique_id, field_version, field_reserved}
	
    local MAGIC_CODE_TRPC = "0930"
    local PROTO_HEADER_LENGTH = 16

    local tcp_src_port     = Field.new("tcp.srcport")
    local tcp_dst_port     = Field.new("tcp.dstport")
    local tcp_stream       = Field.new("tcp.stream")

    local data_dissector     = Dissector.get("data")
    local protobuf_dissector = Dissector.get("protobuf")

    function trpc_proto.init()
        -- tcp_stream_id <-> {client_port, server_port}
        stream_map = {}
    end

    function trpc_proto.dissector(buffer, packet, tree)
        packet.cols.protocol:set("tRPC")

        local f_src_port = tcp_src_port()()
        local f_dst_port = tcp_dst_port()()

        -- record which one is client and which one is server
        -- by first tcp syn frame
        stream_n = tcp_stream().value
        if stream_map[stream_n] == nil then
            stream_map[stream_n] = {f_src_port, f_dst_port}
        end

        if f_src_port == stream_map[stream_n][1] then
            packet.private["pb_msg_type"] = "message,trpc.RequestProtocol"
        end
        if f_src_port == stream_map[stream_n][2] then
            packet.private["pb_msg_type"] = "message,trpc.ResponseProtocol"
        end

        local magic_value       = buffer(0, 2)
        local type_value        = buffer(2, 1)
        local stream_value      = buffer(3, 1)
        local total_size_value  = buffer(4, 4)
        local header_size_value = buffer(8, 2)
        local unique_id_value   = buffer(10, 4)
        local version_value     = buffer(14, 1)
        local reserved_value    = buffer(15, 1)
        
        local header_length   = header_size_value:uint()
        local subtree         = tree:add(trpc_proto, buffer(), "tRPC Protocol Data")

        data_dissector:call(buffer, packet, tree)

        if buffer:len() < 16 then
            return
        elseif buffer:len() < 16 + header_length then
            return
        end

        local t = subtree:add(trpc_proto, buffer)
        t:add(field_magic, magic_value)
        t:add(field_type, type_value)
        t:add(field_stream, stream_value)
        t:add(field_total_size, total_size_value)
        t:add(field_header_size, header_size_value)
        t:add(field_unique_id, unique_id_value)
        t:add(field_version, version_value)
        t:add(field_reserved, reserved_value)

        pcall(Dissector.call, protobuf_dissector, buffer(16, header_length):tvb(), packet, subtree)
    end

    -- heuristic
    local function heur_dissect_proto(tvbuf, pktinfo, root)
        if (tvbuf:len() < PROTO_HEADER_LENGTH) then
            return false
        end
	
	    local magic = tvbuf:range(0, 2):bytes():tohex()
        -- for range dissectors
        if magic ~= MAGIC_CODE_TRPC then
            return false
        end
        trpc_proto.dissector(tvbuf, pktinfo, root)
        pktinfo.conversation = trpc_proto

        return true
    end

trpc_proto:register_heuristic("tcp", heur_dissect_proto)

end