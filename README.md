extract-map-parts
=================

Extract-map-parts is a tool using pgplot to handle GMT () multi-segment
files (x,y files where segments are separated with ">" sign at the first
position of a new line).  The tool is able to Wrap/Unwrap files, split
segments, extract a set of segments, invert the direction of segments and
draw new-segments interactivelly.

This program is able to manipulate segments files in the format normally
used by GMT.  It reads pairs of XY files, with segments separated with > in
the first position of a new line.

To compile it you will need:

1) make
2) cmake 
3) X11 libraries
4) pgplot libraries

-- Optionally

1) ccmake is nice to help you configure

To compile it:

% make
(choose your options)
press 'c' 'c' 'g'
% cd build
% make
% make install

To test it, just run the program with the supplied xy file with the
countours for the world.

% extract-map-parts world-border.xy

[]'s

Marcelo Bianchi (2014-17-01)

