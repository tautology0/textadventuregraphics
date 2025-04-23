from PIL import Image, ImageDraw
from struct import unpack
import sys, os, argparse, pathlib

class SALDPClass:
	def __init__(self, infile, number, scale=1):
		self.xlen = 254*scale
		self.ylen = 96*scale
		
		self.x = self.y = self.xoff = self.yoff = 0
		self.ycount = self.cycles = self.size = 0
		self.double = True
		self.instructions = []

		# Image
		palette = [ 0x00, 0x00, 0x00,
						0x00, 0x00, 0xff,
						0xff, 0x00, 0x00,
						0xff, 0x00, 0xff,
						0x00, 0xff, 0x00,
						0x00, 0xff, 0xff,
						0xff, 0xff, 0x00,
						0xff, 0xff, 0xff]						
		self.image = Image.new("P", (self.xlen, self.ylen))
		self.image.putpalette(palette)
		
		self.process = True
		ox = oy = 0
		img=ImageDraw.Draw(self.image)
		self.active = False
		while (self.process):
			byte=ord(infile.read(1))
			match byte:
				case 0xc0:
					y=(200-ord(infile.read(1))-9)*scale
					x=ord(infile.read(1))*scale
					ox=x
					oy=y
					self.instructions.append(f"MOVE {x},{y}")
				case 0xc1:
					c=ord(infile.read(1))
					y=(200-ord(infile.read(1))-9)*scale
					x=ord(infile.read(1))*scale
					ox=x
					oy=y					
					ImageDraw.floodfill(self.image, (x, y), c)
					self.instructions.append(f"FILL {c}, {x}, {y}")
				case 0xff:
					if self.active: break
					c=ord(infile.read(1))
					oc = 0
					if c == 0: oc = 7
					img.rectangle([(0, 0), (self.xlen-1,self.ylen-1)], fill = c, outline = oc)
					self.active = True
					self.instructions.append(f"IMAGE {c}")
				case _:
					y = (200-byte-9)*scale
					x = (ord(infile.read(1)))*scale
					img.line([(ox, oy), (x, y)], fill = oc, width = 0)
					ox = x
					oy = y
					self.instructions.append(f"DRAW {x}, {y}")
		
		infile.seek(-1, os.SEEK_CUR)
		
	def save(self, outfile):
		self.image.save(outfile)
	
	def getinstructions(self):
		return self.instructions

# For auto detection - name is the name, machine is the computer (and variation),
# detect is the offset of "AUTO", offset is the start of data, max is the total number of images
# offsets should be the memory address
games = [
	{ 'name': "The Golden Baton", 'machine': 'ZX Spectrum', 'detect': 0x873a, 'offset': 0x9b8c, 'max': 32 },
	{ 'name': "Time Machine", 'machine': 'ZX Spectrum', 'detect': 0x875f, 'offset': 0x979d, 'max': 44 },
	{ 'name': "Arrow of Death part 1", 'machine': 'ZX Spectrum', 'detect': 0x86b3, 'offset': 0x98e7, 'max': 52 },
	{ 'name': "Arrow of Death part 2", 'machine': 'ZX Spectrum', 'detect': 0x89b7, 'offset': 0x9e8a, 'max': 65 },
	{ 'name': "Escape from Pulsar 7", 'machine': 'ZX Spectrum', 'detect': 0x8b1d, 'offset': 0xa144, 'max': 45 },
	{ 'name': "Circus of Death", 'machine': 'ZX Spectrum', 'detect': 0x871a, 'offset': 0x4555, 'max': 36 },
	{ 'name': "Feasibility Experiment", 'machine': 'ZX Spectrum', 'detect': 0x87bf, 'offset': 0x9918, 'max': 59 },
	{ 'name': "Wizard of Akryz", 'machine': 'ZX Spectrum', 'detect': 0x897d, 'offset': 0x9f5c, 'max': 40 },
	{ 'name': "Perseus and Andromeda", 'machine': 'ZX Spectrum', 'detect': 0x8823, 'offset': 0x9e9e, 'max': 40 },
	{ 'name': "10 Little Indians", 'machine': 'ZX Spectrum', 'detect': 0x87b7, 'offset': 0x99a7, 'max': 63 },
	{ 'name': "Waxworks", 'machine': 'ZX Spectrum', 'detect': 0x88d3, 'offset': 0x9dfd, 'max': 41 },	
]

# Try and detect the game with the input filehandle, if found, set the filehandle to the start of graphics
# Return the maximum number of graphics
# -1 == game not found
def detectgame(fin):
	# allow for multiple filetypes
	typeoffset=0
	match(os.path.splitext(fin.name)[1].lower()):
		case '.sna':
			# Spectrum .SNA file
			# .SNA only stores memory from 0x4000 onwards with the registers at the start
			# so offset = 0x4000 - 0x1b = 0x3fe5
			typeoffset=0x3fe5
	for game in games:
		fin.seek(game['detect'] - typeoffset)
		id=fin.read(4)
		if id == b'AUTO':
			# found it
			print(f"Identified game as {game['name']}")
			fin.seek(game['offset'] - typeoffset)
			return (game['offset'] - typeoffset, game['max'])
	return (-1, -1)

parser=argparse.ArgumentParser(
	prog="readldp",
	description="Renders Scott Adams LDP pictures from Questprobe games"
)

parser.add_argument('-f', '--file', type=argparse.FileType('rb'), help='Input filename')
parser.add_argument('-p', '--offset', type=int, nargs='?', help='Offset to first image')
parser.add_argument('-c', '--count', type=int, nargs='?', help='Count of images to extract')
parser.add_argument('-g', '--graphics', action=argparse.BooleanOptionalAction, default=True, help='Render Graphics')
parser.add_argument('-d', '--dump', action=argparse.BooleanOptionalAction, default=False, help='Dump Instructions')
parser.add_argument('-o', '--output', type=pathlib.Path, nargs='?', default='.', help='Output directory')
parser.add_argument('-s', '--scale', type=int, nargs='?', default=1, help='Scaling to apply')
args = parser.parse_args()

count = 0
offset = args.offset
if offset is None:
	(offset, count) = detectgame(args.file)

if offset == -1:
	print("Could not identify game")
	exit(1)

if args.count is not None:
	count = args.count

args.file.seek(offset)

for i in range(1,count+1):
	print(f"Processing image {i}")
	img=SALDPClass(args.file, i, scale=args.scale)
	if args.graphics:
		img.save(args.output / f'image{i}.png')
	if args.dump:
		with open(args.output / f'image{i}.txt', 'w') as outfile:
			outfile.write('\n'.join(img.getinstructions()))
	
args.file.close()