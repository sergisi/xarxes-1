# xarxes-1
Xarxes pràctica 1

TODO:
    solve bug #1 that rewrites the last line when get-conf (and maybe send-conf?).


----
Hola,

Sóc el Sergi Simón (GM3) i t'escric perquè he detectat un error en l'enunciat de la pràctica i potser una falta d'informació en el debugging o falta d'enviar un paquet.

L'error és que en la part del client, tant en el diagrama de temps com en la taula de paquets dels protocols "get-conf" i "send-conf" surt com a paquet de començament del protocol el tipus GET_FILE i SEND_FILE (0x30 i 0x20 respectivament), mentre que en el servidor, en els seus diagrames de temps son SEND_CONF i GET_CONF.

En la part del debug (-d 9), tot i que mostra com ha acceptat les peticions no mostra que hagi enviat el paquest de GET_ACK (no ho he provat amb el SEND encara). A més a més, el meu client no detecta que hagi rebut un paquet. 

---
Coses a preguntar al professor:
    que significa tenir tres paquets sense contestar (espero a rebre la resposta    amb el que fa 3 o no?)
    Si m'envia un REGISTER_NACK he d'esperar la resta de segons per a tornar a enviar el paquet?

# Coses a tenir en compte
Quan s'agafa un client de clients només es canvia si
es torna a assignar al diccionari després.
Això permet agafar sempre estats complets si s'agafar un client, 
es canvia tot el necessari i s'escriu el client, però pot
suposar manternir problemes de carrera.

Per a solucionar-ho faré que les lectures sempre siguin
a nivell de client però que les modificacions siguin a nivell de
clients.