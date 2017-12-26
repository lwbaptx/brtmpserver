#include "rtmpservice.hpp"

namespace brtmpserver {

RtmpStreamManager::RtmpStreamManager() {

}

RtmpStreamManager::~RtmpStreamManager() {

}

void RtmpStreamManager::AddPublisher(const std::string &stream_name, RtmpServerStreamImpl *publisher) {
    _streams[stream_name] = publisher;
}

void RtmpStreamManager::DelPublisher(const std::string &stream_name) {
    std::map<std::string, RtmpServerStreamImpl *>::iterator it = _streams.find(stream_name);
    if (it != _streams.end()) {
        _streams.erase(it);
    }
}

RtmpServerStreamImpl *RtmpStreamManager::FindPublisher(const std::string &stream_name) {
    std::map<std::string, RtmpServerStreamImpl *>::iterator it = _streams.find(stream_name);
    if (it != _streams.end())
        return it->second;
    return NULL;
}

RtmpServerStreamImpl::RtmpServerStreamImpl() {
    _publisher = NULL;
    _is_publish = false;
    _aac_header_sended = false;
    _avc_header_sended = false;
};

RtmpServerStreamImpl::~RtmpServerStreamImpl() {
    LOG(INFO) << "~RtmpServerStreamImpl " << (_is_publish);
    if (_publisher) {
        _publisher->_players.erase(this);
    }

    if (_is_publish) {
        butil::get_leaky_singleton<RtmpStreamManager>()->DelPublisher(_stream_name);
        std::set<RtmpServerStreamImpl *>::iterator it = _players.begin();
        for (; it != _players.end(); ++it) {
            (*it)->_publisher = NULL;
        }
        _players.clear();
    }
};

void RtmpServerStreamImpl::OnPlay(const brpc::RtmpPlayOptions &options,
                                  butil::Status *status,
                                  google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    LOG(INFO) << "OnPlay " << options.stream_name;
    _stream_name = options.stream_name;
    RtmpServerStreamImpl *publisher = butil::get_leaky_singleton<RtmpStreamManager>()->FindPublisher(options.stream_name);
    if (publisher) {
        _publisher = publisher;
        _publisher->_players.insert(this);
    }
}

void RtmpServerStreamImpl::OnPublish(const std::string &stream_name,
                                     brpc::RtmpPublishType publish_type,
                                     butil::Status *status,
                                     google::protobuf::Closure *done) {
    brpc::ClosureGuard done_guard(done);
    LOG(INFO) << "OnPublish " << stream_name;
    _stream_name = stream_name;
    _is_publish = true;
    butil::get_leaky_singleton<RtmpStreamManager>()->AddPublisher(stream_name, this);
}

void RtmpServerStreamImpl::OnAudioMessage(brpc::RtmpAudioMessage *msg) {
    if (msg->IsAACSequenceHeader()) {
        LOG(INFO) << "_aac_sequence_header";
        _aac_sequence_header = *msg;
    }
    std::set<RtmpServerStreamImpl *>::iterator it = _players.begin();
    for (; it != _players.end(); ++it) {
        (*it)->SendAudioMessage(*msg);
    }
}

void RtmpServerStreamImpl::OnVideoMessage(brpc::RtmpVideoMessage *msg) {
    if (msg->IsAVCSequenceHeader()) {
        LOG(INFO) << "_avc_sequence_header";
        _avc_sequence_header = *msg;
    }
    std::set<RtmpServerStreamImpl *>::iterator it = _players.begin();
    for (; it != _players.end(); ++it) {
        (*it)->SendVideoMessage(*msg);
    }
}

int RtmpServerStreamImpl::SendAudioMessage(const brpc::RtmpAudioMessage &msg) {
    if (!_aac_header_sended) {
        _aac_header_sended = true;
        RtmpServerStream::SendAudioMessage(_publisher->_aac_sequence_header);
    }
    return RtmpServerStream::SendAudioMessage(msg);
}

int RtmpServerStreamImpl::SendVideoMessage(const brpc::RtmpVideoMessage &msg) {
    if (!_avc_header_sended) {
        _avc_header_sended = true;
        RtmpServerStream::SendVideoMessage(_publisher->_avc_sequence_header);
    }
    return RtmpServerStream::SendVideoMessage(msg);
}

RtmpServiceImpl::RtmpServiceImpl() {
}

RtmpServiceImpl::~RtmpServiceImpl() {
}

brpc::RtmpServerStream *RtmpServiceImpl::NewStream(const brpc::RtmpConnectRequest &) {
    LOG(INFO) << "NewStream";
    return new RtmpServerStreamImpl();
}

} // namespace example
