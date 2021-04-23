TOGOE-DULCU ANDREEA-CRISTINA 322 CB TEMA 2 PC

	Pentru implementarea temei am creeat fisierele server.c si subscriber.c .
	In fisierul server imi creez doua socket-uri, unul pentru listen, pe care 
voi primi si trimite mesajele de la si catre clientii TCP, iar celalalt pentru 
primirea mesajelor UDP. Am un while care merge pana cand serverul primeste 
comanda "exit", iar in el verific daca pe socket-ul de listen vine o cerere
de conectare. Daca da, acesta creeaza un nou socket pe care il adauga in 
multimea de citire, apoi verific daca cunosc deja id-ul clientului TCP care s-a
conectat. Daca da, atunci ii updatez socket-ul pe care e conectat(in lista de 
clienti) si ii trimit orice mesaje erau stocate pentru acesta, iar daca nu 
atunci il adaug in lista de clienti.
	Daca primesc date pe socket-ul pentru clientii UDP atunci receptionez 
mesajele, le prelucrez si le trimit mai departe clientilor activi care sunt 
abonati la topicurile respective. Pentru clientii inactivi, daca sunt abonati 
la topic iar optiunea de store&forward este "bifata", le voi pastra mesajele 
in lista aferenta lor din lista de clienti.
	Daca primesc un pachet de la un client TCP, verific daca acesta a primit 
comanda "exit". Atunci el devine delogat de pe server. Daca primesc "subscribe 
topic sf" atunci abonez clientul la topicul respectiv adaugand topicul in 
lista de topicuri a clientului, iar daca comanda este de forma "unsubscribe 
topic" atunci ii scot topicul din lista de topicuri.

	In fisierul subscriber, primesc comenzi de la tastatura pe care le trimit 
mai departe catre server. De la server primesc mesaje cu topicuri la care sunt 
abonat, pe care le prelucrez si le afisez. 