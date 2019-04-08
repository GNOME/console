#!/usr/bin/python3

# pip3 install ansicolors --user

from colors import color

print('Colour Table')

for i in range(8):
    print(color(' #%02d ' % i, fg=i), end='')

print()

for i in range(8,16):
    print(color(' #%02d ' % i, fg=i), end='')

print()

for i in range(8):
    print(color(' #%02d ' % i, bg=i), end='')

print()

for i in range(8,16):
    print(color(' #%02d ' % i, bg=i), end='')

print()
