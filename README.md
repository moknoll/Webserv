### TO DO 

**Moritz**
- finish request complete handling ask nokha 
- maybe chenge the server to ipv6 and use getaddrinffo -> Chapter 4 and 5 
- canonical form
 * Notes:
 * - Currently treats any non-negative send() result as success and does not
 *   handle partial writes or EAGAIN/EWOULDBLOCK retries.
 * - No error logging is performed on send() failure; callers should ensure
 *   the client is cleaned up elsewhere if needed.