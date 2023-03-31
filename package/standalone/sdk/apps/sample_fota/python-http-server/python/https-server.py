from http.server import HTTPServer, BaseHTTPRequestHandler, SimpleHTTPRequestHandler
import ssl

bind_to_address = ''
server_port = 4443
ssl_key_file = "ssl-cert/server.key"
ssl_certificate_file = "ssl-cert/server.crt"

#httpd = HTTPServer((bind_to_address, 4443), BaseHTTPRequestHandler)
httpd = HTTPServer((bind_to_address, server_port), SimpleHTTPRequestHandler)

httpd.socket = ssl.wrap_socket (httpd.socket,
                                keyfile=ssl_key_file,
                                certfile=ssl_certificate_file,
                                server_side=True)

print("Serving HTTP on 0.0.0.0 port 4443..")
httpd.serve_forever()
