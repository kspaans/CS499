#Copyright Jon Berg , turtlemeat.com
## Note to self, doublecheck that I'm actually allowed to use this.

import string, cgi, time, threading
from os import curdir, sep
from socket import *
from json import loads
from time import sleep
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
#import pri

class Field():
        def __init__(self):
            self.field = "Hi there!\n"

gamefield = Field()
gennum    = 0

class MyHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        global gamefield
        try:
            if self.path.endswith(".html"):
                f = open(curdir + sep + self.path) #self.path has /test.html
#note that this potentially makes every file on your computer readable by the internet

                self.send_response(200)
                self.send_header('Content-type', 'text/html')
                self.end_headers()
                self.wfile.write(f.read())
                f.close()
                return
            if self.path.endswith(".esp"):   #our dynamic content
                self.send_response(200)
                self.send_header('Content-type', 'text/html')
                self.end_headers()
                self.wfile.write("hey, today is the" + str(time.localtime()[7]))
                self.wfile.write(" day in the year " + str(time.localtime()[0]))
                return
            if self.path.endswith('json'):
                self.send_response(200)
                self.send_header('Content-type', 'text/html')
                self.end_headers()
                self.wfile.write('trololololo %s' % gamefield.field)
                return
            return
        except IOError:
            self.send_error(404,'File Not Found: %s' % self.path)

    def do_POST(self):
        global rootnode
        try:
            ctype, pdict = cgi.parse_header(self.headers.getheader('content-type'))
            if ctype == 'multipart/form-data':
                query=cgi.parse_multipart(self.rfile, pdict)
            self.send_response(301)
            self.end_headers()
            upfilecontent = query.get('upfile')
            print "filecontent", upfilecontent[0]
            self.wfile.write("<HTML>POST OK.<BR><BR>");
            self.wfile.write(upfilecontent[0]);
        except :
            pass

def sock2con(sock, addrs):
    global gamefield
    global gennum
    while 1:
        data, addrs[0] = sock.recvfrom(1500)
        #print 'Got json literal:', data
        cells = loads(data)
        print 'JSON decoded:', cells
        html = []
        html.append('Generation %d</br>' % gennum)
        gennum += 1
        html.append('<table>')
        for x in xrange(0, cells['x']):
            html.append('<tr height="1px" width="1px">')
            for y in xrange(0, cells['y']):
                if [x, y] in cells['f']:
                    html.append('<td bgcolor="black"></td>')
                    cells['f'] = filter(lambda x: x in [x, y], cells['f'])
                else:
                    html.append('<td></td>')
            html.append('</tr>\n')
        html.append('</table><hr />\n')
        gamefield.field = "%s%s" % (gamefield.field, "".join(html))

def main():
    try:
        hserver = HTTPServer(('', 8080), MyHandler)
        print 'started httpserver...'
        httpd = threading.Thread(target=hserver.serve_forever)
        srvaddr = ('10.0.0.1', 8889)
        sock = socket(AF_INET, SOCK_DGRAM)
        sock.bind(srvaddr)
        addrs = [None]
        sockt = threading.Thread(target=sock2con, args=(sock, addrs))
        sockt.daemon = True
        httpd.daemon = True
        httpd.start()
        sockt.start()
        while 1:
            sleep(1)

    except KeyboardInterrupt:
        print '^C received, shutting down server'
        hserver.socket.close()

if __name__ == '__main__':
    main()

