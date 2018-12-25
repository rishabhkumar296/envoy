#include "extensions/filters/http/tap/tap_filter.h"

#include "envoy/admin/v2alpha/tap.pb.h"
#include "envoy/admin/v2alpha/tap.pb.validate.h"

#include "common/buffer/buffer_impl.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace TapFilter {

AdminHandler::AdminHandler(Server::Admin& admin, Event::Dispatcher& main_thread_dispatcher)
    : admin_(admin), main_thread_dispatcher_(main_thread_dispatcher) {
  bool rc =
      admin_.addHandler("/tap", "tap filter control", MAKE_ADMIN_HANDLER(handler), true, true);
  RELEASE_ASSERT(rc, "/tap admin endpoint is taken");
}

AdminHandler::~AdminHandler() {
  bool rc = admin_.removeHandler("/tap");
  ASSERT(rc); // fixfix add test
}

Http::Code AdminHandler::handler(absl::string_view, Http::HeaderMap&, Buffer::Instance&,
                                 Server::AdminStream& admin_stream) {
  if (attached_request_.has_value()) {
    ASSERT(false); // fixfix
  }

  if (admin_stream.getRequestBody() == nullptr) {
    ASSERT(false); // fixfix
  }

  envoy::admin::v2alpha::TapRequest tap_request;
  MessageUtil::loadFromYamlAndValidate(admin_stream.getRequestBody()->toString(), tap_request);
  // fixfix error checking on load

  ENVOY_LOG(debug, "tap admin request for config_id={}", tap_request.config_id());
  if (config_id_map_.count(tap_request.config_id()) == 0) {
    ASSERT(false); // fixfix
  }
  for (auto config : config_id_map_[tap_request.config_id()]) {
    config->newTapConfig(std::move(*tap_request.mutable_tap_config()), *this);
  }

  admin_stream.setEndStreamOnComplete(false);
  admin_stream.addOnDestroyCallback([this] {
    for (auto config : config_id_map_[attached_request_.value().config_id_]) {
      ENVOY_LOG(debug, "detach tap admin request for config_id={}",
                attached_request_.value().config_id_);
      config->clearTapConfig();
      attached_request_ = absl::nullopt;
    }
  });
  attached_request_.emplace(tap_request.config_id(), &admin_stream);
  return Http::Code::OK;
}

void AdminHandler::registerConfig(FilterConfig& config, const std::string& config_id) {
  // fixfix asserts
  config_id_map_[config_id].insert(&config);
}

void AdminHandler::unregisterConfig(FilterConfig& config, const std::string& config_id) {
  // fixfix asserts
  config_id_map_[config_id].erase(&config);
  // fixfix remove if empty
}

void AdminHandler::submitBufferedTrace(std::shared_ptr<Protobuf::Message> trace) {
  ENVOY_LOG(debug, "admin submitting buffered trace to main thread");
  main_thread_dispatcher_.post([this, trace]() {
    if (attached_request_.has_value()) {
      ENVOY_LOG(debug, "admin writing buffered trace to response");
      Buffer::OwnedImpl json_trace{MessageUtil::getJsonStringFromMessage(*trace, true, true)};
      attached_request_.value().admin_stream_->getDecoderFilterCallbacks().encodeData(json_trace,
                                                                                      false);
    }
  });
}

// Singleton registration via macro defined in envoy/singleton/manager.h
SINGLETON_MANAGER_REGISTRATION(tap_admin_handler);

FilterConfig::FilterConfig(const envoy::config::filter::http::tap::v2alpha::Tap& proto_config,
                           const std::string& stats_prefix, TapConfigFactoryPtr&& config_factory,
                           Stats::Scope& scope, Server::Admin& admin,
                           Singleton::Manager& singleton_manager, ThreadLocal::SlotAllocator& tls,
                           Event::Dispatcher& main_thread_dispatcher)
    : proto_config_(proto_config), stats_(Filter::generateStats(stats_prefix, scope)),
      config_factory_(std::move(config_factory)), tls_slot_(tls.allocateSlot()) {

  if (proto_config_.has_admin_config()) {
    admin_handler_ = singleton_manager.getTyped<AdminHandler>(
        SINGLETON_MANAGER_REGISTERED_NAME(tap_admin_handler), [&admin, &main_thread_dispatcher] {
          return std::make_shared<AdminHandler>(admin, main_thread_dispatcher);
        });

    admin_handler_->registerConfig(*this, proto_config_.admin_config().config_id());
    ENVOY_LOG(debug, "initializing tap filter with admin endpoint (config_id={})",
              proto_config_.admin_config().config_id());
  } else {
    ENVOY_LOG(debug, "initializing tap filter with no admin endpoint");
  }

  tls_slot_->set([](Event::Dispatcher&) -> ThreadLocal::ThreadLocalObjectSharedPtr {
    return std::make_shared<TlsFilterConfig>();
  });
}

FilterConfig::~FilterConfig() {
  if (admin_handler_) {
    admin_handler_->unregisterConfig(*this, proto_config_.admin_config().config_id());
  }
}

HttpTapConfigSharedPtr FilterConfig::currentConfig() {
  return tls_slot_->getTyped<TlsFilterConfig>().config_;
}

void FilterConfig::clearTapConfig() {
  tls_slot_->runOnAllThreads([this] { tls_slot_->getTyped<TlsFilterConfig>().config_ = nullptr; });
}

void FilterConfig::newTapConfig(envoy::service::tap::v2alpha::TapConfig&& proto_config,
                                TapSink& admin_streamer) {
  HttpTapConfigSharedPtr new_config =
      config_factory_->createHttpConfigFromProto(std::move(proto_config), admin_streamer);
  tls_slot_->runOnAllThreads(
      [this, new_config] { tls_slot_->getTyped<TlsFilterConfig>().config_ = new_config; });
}

FilterStats Filter::generateStats(const std::string& prefix, Stats::Scope& scope) {
  std::string final_prefix = prefix + "tap.";
  return {ALL_TAP_FILTER_STATS(POOL_COUNTER_PREFIX(scope, final_prefix))};
}

Http::FilterHeadersStatus Filter::decodeHeaders(Http::HeaderMap& headers, bool) {
  // fixfix no tapper tests
  if (tapper_ != nullptr) {
    tapper_->onRequestHeaders(headers);
  }
  return Http::FilterHeadersStatus::Continue;
}

Http::FilterHeadersStatus Filter::encodeHeaders(Http::HeaderMap& headers, bool) {
  if (tapper_ != nullptr) {
    tapper_->onResponseHeaders(headers);
  }
  return Http::FilterHeadersStatus::Continue;
}

void Filter::log(const Http::HeaderMap* request_headers, const Http::HeaderMap* response_headers,
                 const Http::HeaderMap*, const StreamInfo::StreamInfo&) {
  if (tapper_ != nullptr) {
    tapper_->onLog(request_headers, response_headers);
  }
}

} // namespace TapFilter
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
