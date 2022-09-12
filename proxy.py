import asyncio
import qtm

from pythonosc import udp_client
# from pythonosc import osc_bundle_builder
from pythonosc import osc_message_builder


client = udp_client.SimpleUDPClient("192.168.6.2", 7562)
# bundle = osc_bundle_builder.OscBundleBuilder(
#     osc_bundle_builder.IMMEDIATELY)
msg = osc_message_builder.OscMessageBuilder(address="/osc-test")
msg.add_arg(2)
msg.add_arg(4.2)
# bundle.add_content(msg.build())
# bundle = bundle.build()
msg = msg.build()

client.send(msg)

exit()
# import serial_asyncio

# class OutputProtocol(asyncio.Protocol):
#     _transport: serial_asyncio.SerialTransport
#     @classmethod
#     def connection_made(cls, transport: serial_asyncio.SerialTransport):
#         cls._transport = transport
#         print('port opened', transport)
#         transport.serial.rts = False  # You can manipulate Serial object via transport
#         transport.write(b'k\n')  # Write serial data via transport

#     @classmethod
#     def data_received(cls, data):
#         print('data received', repr(data))
#         if b'\n' in data:
#             cls._transport.close()

#     @classmethod
#     def connection_lost(cls, exc):
#         print('port closed')
#         cls._transport.loop.stop()

#     @classmethod
#     def pause_writing(cls):
#         print('pause writing')
#         print(cls._transport.get_write_buffer_size())

#     @classmethod
#     def resume_writing(cls):
#         print(cls._transport.get_write_buffer_size())
#         print('resume writing')



def on_packet(packet):
    """ Callback function that is called everytime a data packet arrives from QTM """
    # print("Framenumber: {}".format(packet.framenumber))
    header, markers = packet.get_3d_markers()
    # print("Component info: {}".format(header))
    for marker in markers:
        #print("\t", marker)
        #transport.write(b'\t')
        transport.write(b's\n')
        #transport.write(marker)
    transport.write(b'\n')
    


async def setup():
    """ Main function """
    connection = await qtm.connect("127.0.0.1")
    if connection is None:
        return
    
    await connection.stream_frames(components=["3d"], on_packet=on_packet)


if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    coro = serial_asyncio.create_serial_connection(loop, OutputProtocol, 'COM15', baudrate=115200)
    transport, protocol = loop.run_until_complete(coro)
    on_packet.transport = transport
    asyncio.ensure_future(setup())
    loop.run_forever()
    loop.close()