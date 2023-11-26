// AUTOMATICALLY GENERATED BY NANOPACK. DO NOT MODIFY BY HAND.

import Foundation
import NanoPack

let Text_typeID: TypeID = 2

class Text: Widget {
  let content: String

  init(id: Int32, content: String) {
    self.content = content
    super.init(id: id)
  }

  required init?(data: Data) {
    guard data.readTypeID() == Text_typeID else {
      return nil
    }

    var ptr = 12

    let id: Int32 = data.readUnaligned(at: ptr)
    ptr += 4

    let contentSize = data.readSize(ofField: 1)
    guard let content = data.read(at: ptr, withLength: contentSize) else {
      return nil
    }
    ptr += contentSize

    self.content = content
    super.init(id: id)
  }

  override func data() -> Data? {
    var data = Data()
    data.reserveCapacity(12)

    withUnsafeBytes(of: Int32(Text_typeID)) {
      data.append(contentsOf: $0)
    }

    data.append([0], count: 8)

    data.write(size: 4, ofField: 0)
    data.append(int: id)

    data.write(size: content.lengthOfBytes(using: .utf8), ofField: 1)
    data.append(string: content)

    return data
  }
}
