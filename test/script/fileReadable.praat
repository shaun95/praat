writeInfoLine: "Testing fileReadable()"
assert fileReadable: "../runAllTests.praat"
assert fileReadable: "fileReadable.praat"
assert not fileReadable: "lguianeryv;ikneir;rhuvei.praat"
assert not fileReadable: "lguianeryfgdfghdshdfghhuvei.praat"
assert not fileReadable: "../sys"
assert fileReadable: "../sys/graphics.praat"
assert not fileReadable: "up"
assert fileReadable: "up/fileNames-S-H-up.praat"
appendInfoLine: "OK"