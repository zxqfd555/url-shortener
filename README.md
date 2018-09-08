# URL shortener

URL shortener service. Should be capable of performing two essential operations:

 - get a short link for the provided URL and lifetime, respective to the previous link usage;
 - for a short link, redirect user for the original URL (or format 404 message, if there's no such link or it was expired).

The service should handle high load efficiently, targeting at thousands of RPS.
