# zscore
Small server to get data from a Grunenwald console and provide a simple HTTP server to display the score for volleyball.
This is an example of multiple socket use, with 2 TCP Servers accepting a connection (so TCP listen socket example).
One connection is meant to receive data from a Grunenwald sport console (in an ASCII format decoded from radio data captured by rtl-sdr),
the other one is meant to be an ultra simple HTTP server that will provide a simple document containing an ugly HTML TABLE (doesn't even parse the request, just wait for \r\n\r\n) reflecting the score.
The document has a refresh meta to provide automatic reload every 3 seconds.
This has been designed for adding a score to a live stream of sport event.
