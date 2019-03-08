# xarxes-1
Xarxes pràctica 1

TODO:
    get-conf and send-conf doesn't fill data correcly, making a weird behaviour
    in server 


----
Hola,

Sóc el Sergi Simón (GM3) i t'escric perquè he detectat un error en l'enunciat de la pràctica i potser una falta d'informació en el debugging o falta d'enviar un paquet.

L'error és que en la part del client, tant en el diagrama de temps com en la taula de paquets dels protocols "get-conf" i "send-conf" surt com a paquet de començament del protocol el tipus GET_FILE i SEND_FILE (0x30 i 0x20 respectivament), mentre que en el servidor, en els seus diagrames de temps son SEND_CONF i GET_CONF.

En la part del debug (-d 9), tot i que mostra com ha acceptat les peticions no mostra que hagi enviat el paquest de GET_ACK (no ho he provat amb el SEND encara). A més a més, el meu client no detecta que hagi rebut un paquet. 