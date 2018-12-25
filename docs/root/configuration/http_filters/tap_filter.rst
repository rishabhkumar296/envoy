.. _config_http_filters_tap:

Tap
===

FIXFIX

* :ref:`v2 API reference <envoy_api_msg_config.filter.http.tap.v2alpha.Tap>`
* This filter should be configured with the name *envoy.buffer*.

Statistics
----------

The tap filter outputs statistics in the *http.<stat_prefix>.tap.* namespace. The :ref:`stat prefix
<envoy_api_field_config.filter.network.http_connection_manager.v2.HttpConnectionManager.stat_prefix>` comes from the
owning HTTP connection manager.

.. csv-table::
  :header: Name, Type, Description
  :widths: 1, 1, 2

  rq_timeout, Counter, Total requests that timed out waiting for a full requestfixfix
