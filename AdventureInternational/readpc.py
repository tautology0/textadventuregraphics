from PIL import Image
from struct import unpack
import sys

class SAGAPCGfxFile:
	def __init__(self, filename):
		# Standard variables
		self.x = self.y = self.xoff = self.yoff = 0
		self.ycount = self.cycles = self.size = 0
		self.double = True
		self.xlen = 280
		self.ylen = 158

		# Image
		palette = [ 0x00, 0x00, 0x00,
						0x00, 0xff, 0xff,
						0xff, 0x00, 0xff,
						0xff, 0xff, 0xff ]						
		self.image = Image.new("P", (280, 158))
		self.image.putpalette(palette)
		
		infile = open(filename, "rb")
		magic = infile.read(4)
		if magic != '\xfd\xc2\x88\x04':
			raise ValueError("File does not appear to be valid")
			
		# Size of image data
		infile.seek(0x05)
		self.size = unpack("h", infile.read(2))[0]

		# double flag
		infile.seek(0x0d)
		if ord(infile.read(1)) == 255:
			self.double = False

		# Offsets
		infile.seek(0x0f)
		rawoffset = unpack("h", infile.read(2))[0]
		self.x = self.xoff = ((rawoffset % 80) * 4) - 24
		self.y = self.yoff = rawoffset / 80

		# y length
		infile.seek(0x11)
		self.ylen = unpack("h", infile.read(2))[0]
		self.ylen -= rawoffset
		self.ylen /= 80

		# x length
		infile.seek(0x13)
		self.xlen = ord(infile.read(1)) * 4

		# Start of data
		infile.seek(0x17)
		self.data = infile.read()
		infile.close()
		
		self.outfile = filename + ".png"
		
	def save(self):
		self.image.save(self.outfile)
		
	def putpixel(self, colour):
		self.image.putpixel((self.x, self.y), colour)
		self.x += 1

def drawpixels(img, pattern):
	pixels = []
	
	pixels.append((pattern & 0xc0) >> 6)
	pixels.append((pattern & 0x30) >> 4)
	pixels.append((pattern & 0x0c) >> 2)
	pixels.append((pattern & 0x03))
	
	for pixel in pixels:
		img.putpixel(pixel)
		if not img.double:
			img.putpixel(pixel)

	if img.x >= (img.xlen + img.xoff):
		img.y += 2
		img.x = img.xoff
		img.ycount += 1
		
	if (img.ycount > img.ylen):
		img.y = img.yoff + 1
		img.ycount = 0
		img.cycles += 1
			
for pcfile in sys.argv[1:]:
	print "Processing {}".format(pcfile)
	img=SAGAPCGfxFile(pcfile)

	ptr = 0
	while ptr < img.size:
		count = ord(img.data[ptr])
		ptr += 1

		if (count & 0x80):
			# This is a count
			count = count & 0x7f
			pattern = ord(img.data[ptr])
			ptr += 1
			for c in range(0, count + 1):
				drawpixels(img, pattern)
		else:
			# This is not a count
			for c in range(0, count + 1):
				pattern = ord(img.data[ptr])
				ptr += 1
				drawpixels(img, pattern)
		
		if (img.cycles == 2):
			break
   
	img.save()