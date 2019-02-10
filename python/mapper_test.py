from gnuradio import gr, gr_unittest
from gnuradio import blocks
import ieee802_11_baofdm as baofdm
import ieee802_11

class qa_ieee802_11_baofdm_mapper(gr_unittest.TestCase):
  
  def setUp(self):
    self.tb = gr.top_block()

  def tearDown(self):
    self.tb = None

  def test_001_ieee802_11_baofdm_mapper(self):
    ba_mapper = baofdm.mapper(1, False, 48)
    mapper = ieee802_11.mapper(1, False)
    # test something interesting here

if __name__ == '__main__':
  gr_unittest.run(qa_ieee802_11_baofdm_mapper, "qa_ieee802_11_baofdm_mapper.xml")