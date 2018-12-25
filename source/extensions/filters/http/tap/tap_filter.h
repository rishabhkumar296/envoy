#pragma once

#include "envoy/config/filter/http/tap/v2alpha/tap.pb.h"
#include "envoy/http/filter.h"
#include "envoy/server/admin.h"
#include "envoy/service/tap/v2alpha/common.pb.h"
#include "envoy/singleton/manager.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"
#include "envoy/thread_local/thread_local.h"

#include "extensions/filters/http/tap/tap_config.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace TapFilter {

/**
 * All stats for the tap filter. @see stats_macros.h
 */
// clang-format off
#define ALL_TAP_FILTER_STATS(COUNTER)                                                           \
  COUNTER(rq_tapped)
// clang-format on

/**
 * Wrapper struct for tap filter stats. @see stats_macros.h
 */
struct FilterStats {
  ALL_TAP_FILTER_STATS(GENERATE_COUNTER_STRUCT)
};

class FilterConfig;

/**
 * fixfix
 */
class AdminHandler : public Singleton::Instance, public TapSink, Logger::Loggable<Logger::Id::tap> {
public:
  AdminHandler(Server::Admin& admin, Event::Dispatcher& main_thread_dispatcher);
  ~AdminHandler();

  /**
   * fixfix
   */
  void registerConfig(FilterConfig& config, const std::string& config_id);

  /**
   * fixfix
   */
  void unregisterConfig(FilterConfig& config, const std::string& config_id);

  // TapFilter::TapSink
  void submitBufferedTrace(std::shared_ptr<Protobuf::Message> trace) override;

private:
  struct AttachedRequest {
    AttachedRequest(const std::string& config_id, Server::AdminStream* admin_stream)
        : config_id_(config_id), admin_stream_(admin_stream) {}

    std::string config_id_;
    Server::AdminStream* admin_stream_;
  };

  Http::Code handler(absl::string_view path_and_query, Http::HeaderMap& response_headers,
                     Buffer::Instance& response, Server::AdminStream& admin_stream);

  Server::Admin& admin_;
  Event::Dispatcher& main_thread_dispatcher_;
  std::unordered_map<std::string, std::unordered_set<FilterConfig*>> config_id_map_;
  absl::optional<const AttachedRequest> attached_request_;
};

typedef std::shared_ptr<AdminHandler> AdminHandlerSharedPtr;

/**
 * Configuration for the tap filter.
 */
class FilterConfig : Logger::Loggable<Logger::Id::tap> {
public:
  FilterConfig(const envoy::config::filter::http::tap::v2alpha::Tap& proto_config,
               const std::string& stats_prefix, TapConfigFactoryPtr&& config_factory,
               Stats::Scope& scope, Server::Admin& admin, Singleton::Manager& singleton_manager,
               ThreadLocal::SlotAllocator& tls, Event::Dispatcher& main_thread_dispatcher);
  ~FilterConfig();

  HttpTapConfigSharedPtr currentConfig();
  void clearTapConfig();
  void newTapConfig(envoy::service::tap::v2alpha::TapConfig&& proto_config,
                    TapSink& admin_streamer);
  FilterStats& stats() { return stats_; }

private:
  struct TlsFilterConfig : public ThreadLocal::ThreadLocalObject {
    HttpTapConfigSharedPtr config_;
  };

  const envoy::config::filter::http::tap::v2alpha::Tap proto_config_;
  FilterStats stats_;
  TapConfigFactoryPtr config_factory_;
  ThreadLocal::SlotPtr tls_slot_;
  AdminHandlerSharedPtr admin_handler_;
};

typedef std::shared_ptr<FilterConfig> FilterConfigSharedPtr;

/**
 * fixfix
 */
class Filter : public Http::StreamFilter, public AccessLog::Instance {
public:
  Filter(FilterConfigSharedPtr config)
      : filter_config_(config), tapper_(filter_config_->currentConfig()
                                            ? filter_config_->currentConfig()->newPerRequestTapper()
                                            : nullptr) {}

  static FilterStats generateStats(const std::string& prefix, Stats::Scope& scope);

  // Http::StreamFilterBase
  void onDestroy() override {}

  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::HeaderMap& headers, bool end_stream) override;
  Http::FilterDataStatus decodeData(Buffer::Instance&, bool) override {
    return Http::FilterDataStatus::Continue;
  }
  Http::FilterTrailersStatus decodeTrailers(Http::HeaderMap&) override {
    return Http::FilterTrailersStatus::Continue;
  }
  void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks&) override {}

  // Http::StreamEncoderFilter
  Http::FilterHeadersStatus encode100ContinueHeaders(Http::HeaderMap&) override {
    return Http::FilterHeadersStatus::Continue;
  }
  Http::FilterHeadersStatus encodeHeaders(Http::HeaderMap& headers, bool end_stream) override;
  Http::FilterDataStatus encodeData(Buffer::Instance&, bool) override {
    return Http::FilterDataStatus::Continue;
  }
  Http::FilterTrailersStatus encodeTrailers(Http::HeaderMap&) override {
    return Http::FilterTrailersStatus::Continue;
  }
  void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks&) override {}

  // AccessLog::Instance
  void log(const Http::HeaderMap* request_headers, const Http::HeaderMap* response_headers,
           const Http::HeaderMap* response_trailers,
           const StreamInfo::StreamInfo& stream_info) override;

private:
  FilterConfigSharedPtr filter_config_;
  HttpPerRequestTapperPtr tapper_;
};

} // namespace TapFilter
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
