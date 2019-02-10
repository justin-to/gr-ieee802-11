from gnuradio import gr, gr_unittest
from gnuradio import blocks
import ieee802_11_baofdm as baofdm
import ieee802_11

class qa_ieee802_11_baofdm_decode_mac(gr_unittest.TestCase):
  
  def setUp(self):
    self.tb = gr.top_block()

  def tearDown(self):
    self.tb = None

  def test_001_ieee802_11_baofdm_decode_mac(self):
    ba_decode = baofdm.decode_mac(False, False)
    decode = ieee802_11.decode_mac(False, False)
    # test something interesting here

if __name__ == '__main__':
  gr_unittest.run(qa_ieee802_11_baofdm_decode_mac, "qa_ieee802_11_baofdm_decode_mac.xml")
