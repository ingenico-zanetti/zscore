# zscore
Small server to gets data from a Grunenwald console and provide a simple HTTP server to display the score for volleyball
This is an example of multiple socket use, with 2 TCP Server accepting a connection.
One connection is meant to receive data from a Grunenwald sport console (in an ASCII format with timestamp, decoded from radio data captured by rtl-sdr),
the other one is meant to be an ultra simple HTTP server that will provide a simple document containing an ugly HTML TABLE (doesn't even parse the request, just wait for \r\n\r\n) reflecting the score.
This has been designed for adding a score to a live stream of sport event.
