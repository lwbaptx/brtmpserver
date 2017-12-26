#include "rtmpservice.hpp"

namespace brtmpserver {

RtmpServerStreamImpl::PublishersMapType RtmpServerStreamImpl::_publishers;

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
        PublishersMapType::iterator itPublisher = _publishers.find(_stream_name);
        if (itPublisher != _publishers.end())
            _publishers.erase(itPublisher);
        std::set<RtmpServerStreamImpl *>::iterator itPlayer = _players.begin();
        for (; itPlayer != _players.end(); ++itPlayer) {
            (*itPlayer)->_publisher = NULL;
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
    PublishersMapType::iterator itPublisher = _publishers.find(options.stream_name);
    if (itPublisher != _publishers.end()) {
        _publisher = itPublisher->second;
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
    PublishersMapType::iterator itPublisher = _publishers.find(_stream_name);
    if (itPublisher != _publishers.end()) {
        delete itPublisher->second;
    }
    _publishers[stream_name] = this;
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
