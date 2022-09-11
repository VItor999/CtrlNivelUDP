if [ $1 -le 0 ]; then 
	printf  "Servidor iniciado na porta padrão (20000)"
	./server  20000
else 
	printf  "Servidor iniciado na porta $1"
    ./server $1 
fi 
