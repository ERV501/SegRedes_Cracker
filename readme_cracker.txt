Compilación:	g++ -o cracker cracker_v1.cpp -lpthread

Ejecución:  ./cracker -file "file path" -len "password length"

-El cracker lanza por defecto el máximo número de hilos disponibles para optimizar al máximo la ejecución
-Busca entre las letras minúsculas de [a-z], y con el tamaño de contraseña que se indique en la ejecución
-El tiempo de decodificación de la contraseña para el archivo "trabajos-teoría.pdf.gpg" ha sido de 16min 37s, con 16 hilos

Ejemplo:  ./cracker -file ./trabajos-teoría.pdf.gpg -len 4