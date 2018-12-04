'''
PYTHON 2.X ONLY

Helper functions used in the Bandwidth Adaptive OFDM protocol
'''
import numpy as np

# Maintaining a random state will allow the sender and receiver to coordinate transmissions with a shared seed
class ChannelChooser(object):

  def __init__(self, seed=None):
    self.legal_data_channels = np.asarray(range(-26, -21) + range(-20, -7) + range(-6, 0) + range(1, 7) + range(8, 21) + range(22, 27))
    self.random = np.random.RandomState(seed)

  def permute_and_choose_channels(self, num_channels):
    return self.random.permutation(self.legal_data_channels)[:num_channels]
