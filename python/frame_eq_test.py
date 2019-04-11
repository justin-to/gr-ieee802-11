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
    occupied = (range(6, 11) + range(12, 25) + range(26, 32) + range(33, 39) + range(40, 53) + range(54, 59),)
    freq = 5890000000.0
    band = 10000000.0
    file_path = 'output.bin'

    ba_frame_eq = baofdm.frame_equalizer(1, freq, band, False, False, 64, 48, 4, occupied)
    frame_eq = ieee802_11.frame_equalizer(1, freq, band, False, False)

    src1 = blocks.file_source(8, file_path)
    src2 = blocks.file_source(8, file_path)

    stream1 = blocks.stream_to_vector(8, 64)
    stream2 = blocks.stream_to_vector(8, 64)

    dst1 = blocks.vector_sink_b()
    dst2 = blocks.vector_sink_b()

    vector1 = blocks.vector_to_stream(1, 48)
    vector2 = blocks.vector_to_stream(1, 48)

    self.tb.connect((src1,0), (stream1, 0))
    self.tb.connect((stream1, 0), (ba_frame_eq, 0))
    self.tb.connect((ba_frame_eq, 0), (vector1, 0))
    self.tb.connect((vector1, 0), (dst1, 0))

    self.tb.connect((src2, 0), (stream2, 0))
    self.tb.connect((stream2, 0), (frame_eq, 0))
    self.tb.connect((frame_eq, 0), (vector2, 0))
    self.tb.connect((vector2, 0), (dst2, 0))

    self.tb.run()

    actual = dst1.data()
    expected = dst2.data()

    print("Actual: ", actual)
    print("Expected: ", expected)

    self.assertEqual(actual, expected)
    
if __name__ == '__main__':
  gr_unittest.run(qa_ieee802_11_baofdm_frame_equalizer, "qa_ieee802_11_baofdm_frame_equalizer.xml")
