// AUTOMATICALLY GENERATED BY NANOPACK. DO NOT MODIFY BY HAND.

import { NanoBufReader, NanoBufWriter, type NanoPackMessage } from "nanopack";

class Widget implements NanoPackMessage {
  public static TYPE_ID = 1;

  constructor(public id: number) {}

  public static fromBytes(
    bytes: Uint8Array,
  ): { bytesRead: number; result: Widget } | null {
    const reader = new NanoBufReader(bytes);
    return Widget.fromReader(reader);
  }

  public static fromReader(
    reader: NanoBufReader,
  ): { bytesRead: number; result: Widget } | null {
    let ptr = 8;

    const id = reader.readInt32(ptr);
    ptr += 4;

    return { bytesRead: ptr, result: new Widget(id) };
  }

  public get typeId(): number {
    return 1;
  }

  public bytes(): Uint8Array {
    const writer = new NanoBufWriter(8);
    writer.writeTypeId(1);

    writer.appendInt32(this.id);
    writer.writeFieldSize(0, 4);

    return writer.bytes;
  }

  public bytesWithLengthPrefix(): Uint8Array {
    const writer = new NanoBufWriter(12, true);
    writer.writeTypeId(1);

    writer.appendInt32(this.id);
    writer.writeFieldSize(0, 4);

    writer.writeLengthPrefix(writer.currentSize - 4);

    return writer.bytes;
  }
}

export { Widget };
