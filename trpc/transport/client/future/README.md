This directory contains implementation of future transport.
# Design
- Every client service has its own FutureTransport instance.
- Every reactor of thread model instance has its own FutureTransportAdaptor, in order to avoid cache bouncing.
- FutureConnectorGroup is used to manage all the connections current client service reactor to a backend host.
- Timing wheel is used to improve performance of request timeout.

# Category
- connection complex
- connection pool
- pipeline
