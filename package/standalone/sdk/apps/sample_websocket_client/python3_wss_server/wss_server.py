import asyncio
import websockets
import ssl
import argparse

async def echo(websocket, path):
    # Obtain client's IP address from the WebSocket connection
    ip_address = websocket.remote_address[0]
    print(f"Client connected from IP address: {ip_address}")

    try:
        # Receive messages and echo them back to the client
        async for message in websocket:
            print(f"Received message from {ip_address}: {message}")
            await websocket.send(message)
    except websockets.ConnectionClosed as e:
        print(f"Connection closed with code: {e.code}, reason: {e.reason}")
    finally:
        print("Connection closed")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="WebSocket server with SSL support")
    parser.add_argument('--ssl', action='store_true', help="Use SSL/TLS (wss://)")
    args = parser.parse_args()

    ssl_context = None
    if args.ssl:
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        # Load your SSL certificate and key here
        ssl_context.load_cert_chain(certfile='./server.crt', keyfile='./server.key')

    start_server = websockets.serve(echo, '172.16.200.1', 9000, ssl=ssl_context)

    asyncio.get_event_loop().run_until_complete(start_server)
    asyncio.get_event_loop().run_forever()

