import http.server
import socketserver
import os
import time
import json
import cgi

power = 'Off'
color = '#FF0000'
sRequest = ""
cmdRequest = ""
class RliGhtBRequestHandler(http.server.SimpleHTTPRequestHandler):
        def xsend(self,content,contentType="text/html"):
            print ("serving custom response: " + content)
            self.send_response(200)
            self.send_header("Content-Type", contentType)
            v = len(content)
            self.send_header("Content-Tength", v)
            self.end_headers()
            self.wfile.write(content)

        def do_GET(self):
            global power,color
            sRequest = self.path
            self.path = self.path.split('?',1)[0]
            cmdRequest = sRequest.split('?')
            print ("Request: ")
            print (sRequest)
            print ("Command: ")
            print (cmdRequest)
            if cmdRequest[1] == 'toggle=On%2FOff':
                print ("TOGGLEEEEEEEEEEEEEEEEEEEEEEEE")
                if power == 'Off': 
                    power = 'On'
                    print ("ONNNNNNNNNNNNNNNNNNNNNNNN")
                else: 
                    power = 'Off'
                    print ("OFFFFFFFFFFFFFFFFFFFFFFFF")
                print ("TOGGLEEEEEEEEEEEEEEEEEEEEEEEE")
                self.xsend("ok")
            elif self.path == 'favcolor':
                if color == 'unlatched': color = 'latched'
                else: color = 'unlatched'
                self.xsend("ok")
                print ("COLORRRRRRRRRRRRRRRRRRRRRRRRR")
            elif self.path == '/restart':
                self.xsend("Restarting... please wait a minute or two and refresh")
            elif self.path == '/list':
                v = []
                for f in os.listdir('.'):
                    v.append( { 'type': 'file', 'name': f })
                self.xsend(json.dumps(v),"text/json")
            else:
                print ("return http.server.SimpleHTTPRequestHandler.do_GET(self)")
                return http.server.SimpleHTTPRequestHandler.do_GET(self)
            print ("do_GET(self): ENDDDDDDDDDDDDDDDDDDD")
            
        def do_DELETE(self):
            self.path = self.path.split('?',1)[0]
            print (self.path)
            if self.path == '/edit':
                form = cgi.FieldStorage(
                fp=self.rfile,
                headers=self.headers,
                environ={'REQUEST_METHOD':'POST',
                         'CONTENT_TYPE':self.headers['Content-Type'],
                         })

                for field in form.keys():
                    field_item = form[field]
                    if field == 'path':
                        fn = form[field].value
                        if fn.startswith('/'): fn = fn[1:]
                        if os.path.isfile(fn):
                            os.remove(fn)
                            self.xsend("")
                            return
            self.send_response(404)

        def do_PUT(self):
            self.path = self.path.split('?',1)[0]
            print (self.path)
            if self.path == '/edit':
                form = cgi.FieldStorage(
                fp=self.rfile,
                headers=self.headers,
                environ={'REQUEST_METHOD':'POST',
                         'CONTENT_TYPE':self.headers['Content-Type'],
                         })

                for field in form.keys():
                    field_item = form[field]
                    if field == 'path':
                        fn = form[field].value
                        if fn.startswith('/'): fn = fn[1:]
                        if not os.path.isfile(fn):
                            f = open(fn,'w')
                            f.close()
                            self.xsend("")
                            return
                        else:
                            self.send_response(404)
                            self.end_headers()
                            return

            self.send_response(404)


        def do_POST(self):
            self.path = self.path.split('?',1)[0]
            print (self.path)
            if self.path == '/edit':

                form = cgi.FieldStorage(
                fp=self.rfile,
                headers=self.headers,
                environ={'REQUEST_METHOD':'POST',
                         'CONTENT_TYPE':self.headers['Content-Type'],
                         })

                for field in form.keys():
                    field_item = form[field]
                    if field_item.filename:
                        # The field contains an uploaded file
                        file_data = field_item.file.read()
                        file_len = len(file_data)
                        print('Uploaded %s as "%s" (%d bytes)' % \
                                (field, field_item.filename, file_len))
                        if field == 'data':
                            fn = field_item.filename
                            if fn.startswith('/'): fn = fn[1:]
                            f = open(fn,'w')
                            f.write(file_data)
                            f.close()
                            print ('wrote:'+fn)
                        del file_data
                    else:
                        # Regular form value
                        print('%s=%s' % (field, form[field].value))

                self.xsend("<html><body>ok</body></html>")
            else:
                return http.server.SimpleHTTPRequestHandler.do_GET(self)


os.chdir("c:\Work\RliGhtB\data-uncompressed")
PORT = 80

Handler = RliGhtBRequestHandler

httpd = socketserver.TCPServer(("", PORT), Handler)

print( "serving at port", PORT)
httpd.serve_forever()