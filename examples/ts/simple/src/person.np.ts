// AUTOMATICALLY GENERATED BY NANOPACK. DO NOT MODIFY BY HAND.

import { NanoBufReader, NanoBufWriter } from "nanopack";

class Person {
  public static TYPE_ID = 1;

  constructor(
    public firstName: string,
    public middleName: string | null,
    public lastName: string,
    public age: number,
    public otherFriend: Person | null,
  ) {}

  public static fromBytes(
    bytes: Uint8Array,
  ): { bytesRead: number; result: Person } | null {
    const reader = new NanoBufReader(bytes);
    if (reader.readTypeId() !== Person.TYPE_ID) {
      return null;
    }
    let ptr = 24;

    const firstNameByteLength = reader.readFieldSize(0);
    const firstName = reader.readString(ptr, firstNameByteLength);
    ptr += firstNameByteLength;

    let middleName: string | null;
    if (reader.readFieldSize(1) < 0) {
      middleName = null;
    } else {
      const middleNameByteLength = reader.readFieldSize(1);
      middleName = reader.readString(ptr, middleNameByteLength);
      ptr += middleNameByteLength;
    }

    const lastNameByteLength = reader.readFieldSize(2);
    const lastName = reader.readString(ptr, lastNameByteLength);
    ptr += lastNameByteLength;

    const age = reader.readInt32(ptr);
    ptr += 4;

    const otherFriendSize = reader.readFieldSize(4);
    let otherFriend!: Person | null;
    if (otherFriendSize < 0) {
      otherFriend = null;
    } else {
      const maybe_otherFriend = Person.fromBytes(bytes.subarray(ptr));
      if (!maybe_otherFriend) {
        return null;
      }
      otherFriend = maybe_otherFriend.result;
      ptr += otherFriendSize;
    }

    return {
      bytesRead: ptr,
      result: new Person(firstName, middleName, lastName, age, otherFriend),
    };
  }

  public bytes(): Uint8Array {
    const writer = new NanoBufWriter(24);
    writer.writeTypeId(Person.TYPE_ID);

    const firstNameByteLength = writer.appendString(this.firstName);
    writer.writeFieldSize(0, firstNameByteLength);

    if (this.middleName) {
      const middleNameByteLength = writer.appendString(this.middleName);
      writer.writeFieldSize(1, middleNameByteLength);
    } else {
      writer.writeFieldSize(1, -1);
    }

    const lastNameByteLength = writer.appendString(this.lastName);
    writer.writeFieldSize(2, lastNameByteLength);

    writer.appendInt32(this.age);
    writer.writeFieldSize(3, 4);

    if (this.otherFriend) {
      const otherFriendData = this.otherFriend.bytes();
      writer.appendBytes(otherFriendData);
      writer.writeFieldSize(4, otherFriendData.byteLength);
    } else {
      writer.writeFieldSize(4, -1);
    }

    return writer.bytes;
  }
}

export { Person };