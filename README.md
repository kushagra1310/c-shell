## Directions to run
### Shell:
1) Run ```make all``` inside the /shell directory
2) Run ```./shell.out```
### Networking:
1) Compile server.c using ```gcc server.c -o server```
2) Compile client.c using ```gcc client.c -o client```
3) Run ```./server <port> [--chat] [loss_rate]``` (based on your modes of operation in the placeholders)
4) Run  ```./client <server_ip> <server_port> <input_file> <output_file_name> [loss_rate]``` for file transfer mode, or ```./client <server_ip> <server_port> --chat [loss_rate]``` for chat mode.
