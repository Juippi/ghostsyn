#!/usr/bin/python3

import fileinput
import json
import argparse


def convert_channels(song, num_tracks):
    for p in song["patterns"]:
        old_tracks = p["num_tracks"]
        p["num_tracks"] = num_tracks
        if num_tracks <= old_tracks:
            p["tracks"] = p["tracks"][:num_tracks]
        else:
            for _ in range(old_tracks, num_tracks):
                p["tracks"].append([[-1, 0, 0] for _ in range(p["num_rows"])])
    song["num_tracks"] = num_tracks
    return song


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Ghostsyn song conversion tool")
    parser.add_argument("song", nargs=1, type=str)
    parser.add_argument("-t", "--tracks", type=int,
                        help="Convert all patterns in song to N tracks")
    args = parser.parse_args()

    song = json.loads(open(args.song[0]).read())
    if args.tracks:
        convert_channels(song, args.tracks)
    print(json.dumps(song))
