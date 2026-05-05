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
- Remember back in the section about send() , above, when I said that send() might not send all the 
bytes you asked it to? That is, you want it to send 512 bytes, but it returns 412. What happened to the
remaining 100 bytes?