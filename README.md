# DNS-server
This is a project i worked on during my studies at the University of Melbourne. This project is for the subject Computer Systems
## Process
This project is a DNS server that works as below <br/>
It takes an IP and port number as input and runs a server and begins listening for queries. It then forwards that query to the input ip and port, receives the response and sends that response to the client.
<br/> This basic DNS server also has a cache, which it firsts attempt to find the queried address in the cache, if it doesn't exist, it continues the process, and stores the reponse in the cache.
