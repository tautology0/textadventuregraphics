#!/usr/bin/env python
import sys
import os
import argparse
import pathlib
from PIL import Image, ImageDraw
from struct import unpack


class SALDPClass:
	def __init__(self, infile, number, palette, scale=1):
		self.xlen = 254*scale
		self.ylen = 96*scale
		
		self.x = self.y = self.xoff = self.yoff = 0
		self.ycount = self.cycles = self.size = 0
		self.double = True
		self.instructions = []

		# Image					
		self.image = Image.new('P', (self.xlen, self.ylen))
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
					self.instructions.append(f'MOVE {x},{y}')
				case 0xc1:
					c=ord(infile.read(1))
					y=(200-ord(infile.read(1))-9)*scale
					x=ord(infile.read(1))*scale
					ox=x
					oy=y					
					ImageDraw.floodfill(self.image, (x, y), c)
					self.instructions.append(f'FILL {c}, {x}, {y}')
				case 0xff:
					if self.active: break
					c=ord(infile.read(1))
					oc = 0
					if c == 0: oc = 7
					img.rectangle([(0, 0), (self.xlen-1,self.ylen-1)], fill = c, outline = oc)
					self.active = True
					self.instructions.append(f'IMAGE {c}')
				case _:
					y = (200-byte-9)*scale
					x = (ord(infile.read(1)))*scale
					img.line([(ox, oy), (x, y)], fill = oc, width = 0)
					ox = x
					oy = y
					self.instructions.append(f'DRAW {x}, {y}')
		
		infile.seek(-1, os.SEEK_CUR)
		
	def save(self, outfile):
		self.image.save(outfile)
	
	def getinstructions(self):
		return self.instructions

# For auto detection - name is the name, machine is the computer (and variation),
# detect is the offset of 'AUTO', offset is the start of data, max is the total number of images
# offsets should be the memory address
games = [
	{'name': 'The Golden Baton', 'machine': 'ZX Spectrum', 'detect': 0x873a, 'offset': 0x9b8c, 'max': 32},
	{'name': 'Time Machine', 'machine': 'ZX Spectrum', 'detect': 0x875f, 'offset': 0x979d, 'max': 44},
	{'name': 'Arrow of Death part 1', 'machine': 'ZX Spectrum', 'detect': 0x86b3, 'offset': 0x98e7, 'max': 52},
	{'name': 'Arrow of Death part 2', 'machine': 'ZX Spectrum', 'detect': 0x89b7, 'offset': 0x9e8a, 'max': 65},
	{'name': 'Escape from Pulsar 7', 'machine': 'ZX Spectrum', 'detect': 0x8b1d, 'offset': 0xa144, 'max': 45},
	{'name': 'Circus of Death', 'machine': 'ZX Spectrum', 'detect': 0x871a, 'offset': 0x4555, 'max': 36},
	{'name': 'Feasibility Experiment', 'machine': 'ZX Spectrum', 'detect': 0x87bf, 'offset': 0x9918, 'max': 59},
	{'name': 'Wizard of Akryz', 'machine': 'ZX Spectrum', 'detect': 0x897d, 'offset': 0x9f5c, 'max': 40},
	{'name': 'Perseus and Andromeda', 'machine': 'ZX Spectrum', 'detect': 0x8823, 'offset': 0x9e9e, 'max': 40},
	{'name': '10 Little Indians', 'machine': 'ZX Spectrum', 'detect': 0x87b7, 'offset': 0x99a7, 'max': 63},
	{'name': 'Waxworks', 'machine': 'ZX Spectrum', 'detect': 0x88d3, 'offset': 0x9dfd, 'max': 41},
	{'name': 'Waxworks', 'machine': 'C64', 'detect': 0x69d1, 'offset': 0x7f2f, 'max': 41}
]

# Try and detect the game with the input filehandle, if found, set the filehandle to the start of graphics
# Return the maximum number of graphics
# -1 == game not found
def detectgame(fin):
	# allow for multiple filetypes
	typeoffset=0
	match(os.path.splitext(fin.name)[1].lower()):
		case '.sna':
			if os.path.getsize(fin.name) == 49179:
				print('Identified type of game as 48K Spectrum SNA file')
				# Spectrum .SNA file
				# .SNA only stores memory from 0x4000 onwards with the registers at the start
				# so offset = 0x4000 - 0x1b = 0x3fe5
				typeoffset = -0x3fe5
				palette = 'spectrum'
			elif os.path.getsize(fin.name) == 120952:
				# Oric unsupported for now
				print('Identified type of game as Oric SNA file')
				typeoffset = 0x24
				palette = 'spectrum'
		case '.vsf':
			print('Identified type of game as Commodore 64')
			# Find the C64MEM module
			fin.seek(0x3a)
			modulename=''
			while modulename != b'C64MEM':
				modulename = fin.read(16).strip(b'\0')
				moduleversion = fin.read(2)
				data = fin.read(4)
				modulelength = unpack('<l', data)[0]
				print(modulename, " ", hex(fin.tell()), " ", data, " ", modulelength)
				if modulename != b'C64MEM':
					fin.seek((modulelength - 22), os.SEEK_CUR)
			typeoffset = fin.tell() + 4
	for game in games:
		fin.seek(game['detect'] + typeoffset)
		id = fin.read(4)
		if id == b'AUTO':
			# found it
			print(f"Identified game as {game['name']}")
			fin.seek(game['offset'] + typeoffset)
			return (game['offset'] + typeoffset, game['max'], 'spectrum')
	return (-1, -1, -1)

def main():
	# Set up palettes
	palettes = {}
	palettes['spectrum'] = [
		0x00, 0x00, 0x00,
		0x00, 0x00, 0xc0,
		0xc0, 0x00, 0x00,
		0xc0, 0x00, 0xc0,
		0x00, 0xc0, 0x00,
		0x00, 0xc0, 0xc0,
		0xc0, 0xc0, 0x00,
		0xc0, 0xc0, 0xc0
	]	
	palettes['c64'] =	[ 
		0x00, 0x00, 0x00,
		0x48, 0x3a, 0xaa,
		0x92, 0x4a, 0x40,
		0x93, 0x51, 0xb6,
		0x72, 0xb1, 0x4b,
		0x84, 0xc5, 0xcc,
		0xd5, 0xdf, 0x7c,
		0xff, 0xff, 0xff
	]

	parser=argparse.ArgumentParser(
		prog='readldp',
		description='Renders Scott Adams LDP pictures from Questprobe games'
	)

	parser.add_argument('-f', '--file', type=argparse.FileType('rb'), help='Input filename')
	parser.add_argument('-q', '--offset', type=int, nargs='?', help='Offset to first image')
	parser.add_argument('-c', '--count', type=int, nargs='?', help='Count of images to extract')
	parser.add_argument('-g', '--graphics', action=argparse.BooleanOptionalAction, default=True, help='Render Graphics')
	parser.add_argument('-d', '--dump', action=argparse.BooleanOptionalAction, default=False, help='Dump Instructions')
	parser.add_argument('-o', '--output', type=pathlib.Path, nargs='?', default='.', help='Output directory')
	parser.add_argument('-s', '--scale', type=int, nargs='?', default=1, help='Scaling to apply')
	parser.add_argument('-p', '--palette', choices=['spectrum','c64'], default='spectrum')
	args = parser.parse_args()

	count = 0
	autopalette = ''
	offset = args.offset
	if offset is None:
		(offset, count, autopalette) = detectgame(args.file)
	
	# palette - if args.palette is set use, that else autopalette, else 'spectrum'
	palette = palettes['spectrum']
	if args.palette is not None:
		palette = palettes[args.palette]
	elif autopalette != '':
		palette = palettes[autopalette]

	if offset == -1:
		print('Could not identify game')
		exit(1)

	if args.count is not None:
		count = args.count

	# Create the output directory if it doesn't exist
	args.output.mkdir(exist_ok = True)
	args.file.seek(offset)

	for i in range(1,count+1):
		print(f'Processing image {i}')
		img=SALDPClass(args.file, i, palette, scale=args.scale)
		if args.graphics:
			img.save(args.output / f'image{i}.png')
		if args.dump:
			with open(args.output / f'image{i}.txt', 'w') as outfile:
				outfile.write('\n'.join(img.getinstructions()))
		
	args.file.close()
	
if __name__ == '__main__':
	main()