#!/usr/bin/env python3
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import argparse
import time

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        counter = Path(self.server.counter_file)
        try:
            count = int(counter.read_text())
        except Exception:
            count = 0
        counter.write_text(str(count + 1))

        if self.path.startswith('/slow'):
            time.sleep(0.4)
        body = f"origin response for {self.path}\n".encode()
        self.send_response(200)
        self.send_header('Content-Type', 'text/plain')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        pass

parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=18080)
parser.add_argument('--counter', required=True)
args = parser.parse_args()

Path(args.counter).write_text('0')
server = ThreadingHTTPServer(('127.0.0.1', args.port), Handler)
server.counter_file = args.counter
server.serve_forever()
