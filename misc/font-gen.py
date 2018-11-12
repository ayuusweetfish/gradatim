# Put with Kite One under the same directory and run with
# fontforge -lang=py -script font-gen.py

import fontforge
import psMat
import math

font = fontforge.open('KiteOne-Regular.ttf')
print(font.copyright)
print(font.familyname)
print(font.fontname)
print(font.fullname)

font.selection.all()
skew_mat = psMat.skew(-7 / 180.0 * math.pi)
font.transform(skew_mat)

font.selection.select(('ranges', None), '0', '9')
skew_mat = psMat.skew(+1.2 / 180.0 * math.pi)
font.transform(skew_mat)

font.copyright += "; Modified and renamed to 'Tako'"
font.familyname = 'Tako Zero'
font.fontname = 'TakoZero-Irregular'
font.fullname = 'Tako Zero'
font.generate('TakoZero-Irregular.ttf')
