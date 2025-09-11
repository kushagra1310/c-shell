[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/fkrRRp25)
## ASSUMPTIONS
### Networking:
resend ack/fin/other stuff not required as data packets are mentioned not control packets
sliding window not used for chat mode, instead retransmitting after 200 ms, also message length limit is 1024 bytes
### xv6:
Logging not done as printf is slow and messes up scheduler. Other mentioned in report
