//
//  PadButton.swift
//  YabaSnashiro
//
//  Created by Shinya Miyamoto on 2024/07/20.
//  Copyright © 2024 devMiyax. All rights reserved.
//

import Foundation

import UIKit

@objc enum PadButtons: Int {
    case up = 0
    case right = 1
    case down = 2
    case left = 3
    case rightTrigger = 4
    case leftTrigger = 5
    case start = 6
    case a = 7
    case b = 8
    case c = 9
    case x = 10
    case y = 11
    case z = 12
    case last = 13
}

@objcMembers
class PadButton: NSObject {
    var target: UIView?
    var _isOn: UITouch?
    var pointId: UITouch?

    override init() {
        super.init()
        self._isOn = nil
    }

    func on(_ index: UITouch) {
        self._isOn = index
    }

    func off() {
        self._isOn = nil
    }

    func isOn() -> Bool {
        return self._isOn != nil
    }

    func isOn(_ index: UITouch) -> Bool {
        return self._isOn != index
    }

    func getPointId() -> UITouch? {
        return self._isOn
    }
}


