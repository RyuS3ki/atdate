# Aptel-2
Proyecto Aptel 2017


Lab HTTP

/usr/local/apache2.2/bin/httpd -f ~/labhttp/httpd.conf
/usr/local/apache2.2/bin/httpd -k restart -f ~/labhttp/httpd.conf

telnet 192.100.100.119 18649

GET / HTTP/0.9

HTTP/1.1 200 OK

GET / HTTP/1.0

HTTP/1.1 200 OK

GET / HTTP/1.1           

HTTP/1.1 400 Bad Request

GET / HTTP/1.1
Host: 192.100.100.119:18649    <- Obligatorio en 1.1

HTTP/1.1 200 OK

aptel:aplicaciones -> YXB0ZWw6YXBsaWNhY2lvbmVz
