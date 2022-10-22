"""
  Qualisys Track Manager (QTM) read example.

  Requires python 3.8 or later.
  Dependencies: qtm

  pip install qtm

  This example just connects to QTM on the local PC and streams the 3D positional data.

  See also: https://qualisys.github.io/qualisys_python_sdk/index.html
"""

import asyncio
from typing import List, Tuple
import qtm


class PositionHistory:
    """
    Class to keep track of the last N positions of a marker

    Any new positions added to the history will overwrite the oldest position.
    """

    def __init__(self, size: int, id: int):
        """
        Create a new PositionHistory object

        :param size: The number of positions to keep track of
        :param id: The id of the marker
        """

        self.size: int = size
        self.id: int = id
        self.loc: int = 0
        self.filled: bool = False
        self.positions: List[Tuple[float, float, float]] = []

    def add(self, position: Tuple[float, float, float]):
        """
        Add a new position to the history

        :param position: The position to add
                         tuple of (x, y, z) coordinates
        """

        if not self.filled:
            item_count: int = len(self.positions)
            if item_count < self.size:
                self.positions.append(position)
                self.loc = item_count
                if item_count == self.size - 1:
                    self.filled = True
            return

        self.loc = (self.loc + 1) % self.size
        self.positions[self.loc] = position

    def get(self, index: int) -> Tuple[float, float, float]:
        """ 
        Get a position from the history
        
        :param index: The index of the position to get
        """
        return self.positions[index]

    def get_last(self) -> Tuple[float, float, float]:
        """ Get the last position added to the history """
        return self.positions[self.loc]

    def get_first(self) -> Tuple[float, float, float]:
        """ Get the first _available_ position added to the history """
        loc = (self.loc + 1) % self.size
        return self.positions[loc]


# QTM server IP address
QTM_SERVER: str = "127.0.0.1"

# List of markers to track.
MARKER_INDICES: tuple = (
    0,
    1,
)

# how many previous positions to keep
POSITION_HISTORY_LENGTH: int = 2

position_history: List[PositionHistory]



def on_packet(packet: qtm.protocol.QRTPacket) -> None:
    """ Callback function that is called everytime a data packet arrives from QTM """

    markers: List[qtm.packet.RT3DMarkerPosition]
    _, markers = packet.get_3d_markers()

    for i in range(len(markers)):
        position_history[i].add((markers[i].x, markers[i].y, markers[i].z))
        


async def setup() -> None:
    """ Main function """
    connection: qtm.QRTConnection = await qtm.connect(QTM_SERVER)
    if connection is None:
        return
    for marker_idx in MARKER_INDICES:
        position_history.append(PositionHistory(POSITION_HISTORY_LENGTH, marker_idx))
    await connection.stream_frames(components=["3d"], on_packet=on_packet)


if __name__ == "__main__":
    asyncio.ensure_future(setup())
    asyncio.get_event_loop().run_forever()
