from gnuradio import gr, gr_unittest
from gnuradio import blocks
import ieee802_11_baofdm as baofdm
import ieee802_11

class qa_ieee802_11_baofdm_frame_equalizer(gr_unittest.TestCase):
  
  def setUp(self):
    self.tb = gr.top_block()

  def tearDown(self):
    self.tb = None

  def test_001_ieee802_11_baofdm_frame_equalizer(self):
    occupied = (range(-26, -21) + range(-20, -7) + range(-6, 0) + range(1, 7) + range(8, 21) + range(22, 27),)
    freq = 5890000000.0
    band = 10000000.0
    ba_frame_eq = baofdm.frame_equalizer(1, freq, band, False, False, 64, 48, 4, occupied)
    frame_eq = ieee802_11.frame_equalizer(1, freq, band, False, False)
    # test something interesting here

if __name__ == '__main__':
  gr_unittest.run(qa_ieee802_11_baofdm_frame_equalizer, "qa_ieee802_11_baofdm_frame_equalizer.xml")