### TO DO 

**Moritz**
- finish request complete handling ask nokha 
- canonical form
 * Notes:
 * - Currently treats any non-negative send() result as success and does not
 *   handle partial writes or EAGAIN/EWOULDBLOCK retries.
 * - No error logging is performed on send() failure; callers should ensure
 *   the client is cleaned up elsewhere if needed.
 - after MVP add server_name check
