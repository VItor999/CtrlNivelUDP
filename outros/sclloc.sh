if [$1 -le 0]; then
	printf "Cliente local padrão iniciado \n Porta: 20000 IP: localhost\n"
	./client 127.0.0.1 20000
else 
	printf "Cliente local padrão iniciado \n Porta:$2 IP: $1\n"
	./client $1 $2
fi
