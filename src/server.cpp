// Copyright (c) 2014 Baidu, Inc.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// A simple rtmpserver

#include <set>
#include <map>
#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include <brpc/rtmp.pb.h>
#include <brpc/rtmp.h>
#include <butil/memory/singleton_on_pthread_once.h>

DEFINE_bool(echo_attachment, true, "Echo attachment as well");
DEFINE_int32(port, 1935, "rtmp Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state "
             "(waiting for client to close connection before server stops)");

// Your implementation of brpc::RtmpService
namespace example {

class RtmpServerStreamImpl;

class RtmpStreamManager {
public:
    void AddPublisher(const std::string& stream_name, RtmpServerStreamImpl* publisher) {
        _streams[stream_name] = publisher;
    }
    void DelPublisher(const std::string& stream_name) {
        std::map<std::string, RtmpServerStreamImpl*>::iterator it = _streams.find(stream_name);
        if (it != _streams.end()) {
            _streams.erase(it);
        }
    }
    RtmpServerStreamImpl* FindPublisher(const std::string& stream_name) {
        std::map<std::string, RtmpServerStreamImpl*>::iterator it = _streams.find(stream_name);       
        if (it != _streams.end())
            return it->second;
        return NULL;
    }
private:
    std::map<std::string, RtmpServerStreamImpl*> _streams;
};

class RtmpServerStreamImpl : public brpc::RtmpServerStream {
public:
    RtmpServerStreamImpl() {
        _publisher = NULL;
        _is_publish = false;
        _aac_header_sended = false;
        _avc_header_sended = false;
    };
    virtual ~RtmpServerStreamImpl() {
        LOG(INFO) << "~RtmpServerStreamImpl " << (_is_publish);
        if (_publisher) {
            _publisher->DelPlayer(this);
        }

        if (_is_publish) {
            butil::get_leaky_singleton<RtmpStreamManager>()->DelPublisher(_stream_name);
            std::set<RtmpServerStreamImpl*>::iterator it = _players.begin();
            for (; it != _players.end(); ++it) {
                (*it)->OnPublisherClosed();
            }
            _players.clear();
        }
    };
    virtual void OnPlay(const brpc::RtmpPlayOptions& options,
                        butil::Status* status,
                        google::protobuf::Closure* done) {
        brpc::ClosureGuard done_guard(done);
        LOG(INFO) << "OnPlay " << options.stream_name;
        _stream_name = options.stream_name;
        RtmpServerStreamImpl* publisher = butil::get_leaky_singleton<RtmpStreamManager>()->FindPublisher(options.stream_name);
        if (publisher) {
            _publisher = publisher;
            publisher->AddPlayer(this);
        }
    }
    virtual void OnPublish(const std::string& stream_name,
                            brpc::RtmpPublishType publish_type,
                            butil::Status* status,
                           google::protobuf::Closure* done) {
        brpc::ClosureGuard done_guard(done);
        LOG(INFO) << "OnPublish " << stream_name;
        _stream_name = stream_name;
        _is_publish = true;
        butil::get_leaky_singleton<RtmpStreamManager>()->AddPublisher(stream_name, this);
    }
    virtual void OnAudioMessage(brpc::RtmpAudioMessage* msg) {
        if (msg->IsAACSequenceHeader()) {
            LOG(INFO) << "_aac_sequence_header";
            _aac_sequence_header = *msg;
        }       
        std::set<RtmpServerStreamImpl*>::iterator it = _players.begin();
        for (; it != _players.end(); ++it) {
            (*it)->SendAudioMessage(*msg);
        }
    }

    virtual void OnVideoMessage(brpc::RtmpVideoMessage* msg) {
        if (msg->IsAVCSequenceHeader()) {
            LOG(INFO) << "_avc_sequence_header";
            _avc_sequence_header = *msg;
        }
        std::set<RtmpServerStreamImpl*>::iterator it = _players.begin();
        for (; it != _players.end(); ++it) {
            (*it)->SendVideoMessage(*msg);
        }
    }

    virtual int SendAudioMessage(const brpc::RtmpAudioMessage& msg) {
        if (!_aac_header_sended) {
            _aac_header_sended = true;
            RtmpServerStream::SendAudioMessage(*_publisher->GetAACSequenceHeader());
        }
        return RtmpServerStream::SendAudioMessage(msg);
    }

    virtual int SendVideoMessage(const brpc::RtmpVideoMessage& msg) {
        if (!_avc_header_sended) {
            _avc_header_sended = true;
            RtmpServerStream::SendVideoMessage(*_publisher->GetAVCSequenceHeader());
        }
        return RtmpServerStream::SendVideoMessage(msg);
    }

public:
    void AddPlayer(RtmpServerStreamImpl* player) {
        LOG(INFO) << "AddPlayer ";
        _players.insert(player);
    }
    void DelPlayer(RtmpServerStreamImpl* player) {
        LOG(INFO) << "DelPlayer ";
        _players.erase(player);
    }
    void OnPublisherClosed() {
        _publisher = NULL;
    }
    brpc::RtmpVideoMessage* GetAVCSequenceHeader() {
        return &_avc_sequence_header;
    }
    brpc::RtmpAudioMessage* GetAACSequenceHeader() {
        return &_aac_sequence_header;
    }

private:
    std::set<RtmpServerStreamImpl*> _players;
    RtmpServerStreamImpl* _publisher;
    brpc::RtmpVideoMessage _avc_sequence_header;
    brpc::RtmpAudioMessage _aac_sequence_header;
    bool _aac_header_sended;
    bool _avc_header_sended;
    std::string _stream_name;
    bool _is_publish;
};

class RtmpServiceImpl : public brpc::RtmpService {
public:
    RtmpServiceImpl() {};
    virtual ~RtmpServiceImpl() {};
    virtual brpc::RtmpServerStream* NewStream(const brpc::RtmpConnectRequest&) {
        LOG(INFO) << "NewStream";
        return new RtmpServerStreamImpl();
    }
};
}  // namespace example

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // Generally you only need one Server.
    brpc::Server server;

    // Instance of your service.
    example::RtmpServiceImpl rtmp_service_impl;

    // Start the server.
    brpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    options.rtmp_service = &rtmp_service_impl;
    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
