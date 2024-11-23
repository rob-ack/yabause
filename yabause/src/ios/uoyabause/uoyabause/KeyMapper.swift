//
//  KeyMapper.swift
//  YabaSnashiro
//
//  Created by Shinya Miyamoto on 2024/07/20.
//  Copyright © 2024 devMiyax. All rights reserved.
//

import Foundation

@objc enum KeyMapMappableButton: Int {
    case MFI_BUTTON_X = 0, MFI_BUTTON_A, MFI_BUTTON_B, MFI_BUTTON_Y, MFI_BUTTON_LT, MFI_BUTTON_RT, MFI_BUTTON_LS, MFI_BUTTON_RS, MFI_DPAD_UP, MFI_DPAD_DOWN, MFI_DPAD_LEFT, MFI_DPAD_RIGHT, MFI_BUTTON_HOME, MFI_BUTTON_MENU, MFI_BUTTON_OPTION
}

@objcMembers
class KeyMapper: NSObject, NSCopying {
    private var keyMapping: [KeyMapMappableButton: SaturnKey] = [:]

    override init() {
        super.init()
        loadFromDefaults()
    }

    func loadFromDefaults() {
        guard let data = UserDefaults.standard.object(forKey: "keyMapping") as? Data,
              let fetchedDict = try? NSKeyedUnarchiver.unarchiveTopLevelObjectWithData(data) as? [KeyMapMappableButton: SaturnKey] else {
            keyMapping = defaultMapping()
            return
        }
        keyMapping = fetchedDict
    }

    func copy(with zone: NSZone? = nil) -> Any {
        let copy = KeyMapper()
        copy.keyMapping = self.keyMapping
        return copy
    }

    private func defaultMapping() -> [KeyMapMappableButton: SaturnKey] {
        return [.MFI_BUTTON_A: .a, .MFI_BUTTON_B: .b, .MFI_BUTTON_RS: .c, .MFI_BUTTON_X: .x, .MFI_BUTTON_Y: .y, .MFI_BUTTON_LS: .z, .MFI_BUTTON_LT: .leftTrigger, .MFI_BUTTON_RT: .rightTrigger, .MFI_BUTTON_MENU: .start]
    }

    func resetToDefaults() {
        keyMapping = defaultMapping()
    }

    func saveKeyMapping() {
        if let data = try? NSKeyedArchiver.archivedData(withRootObject: keyMapping, requiringSecureCoding: false) {
            UserDefaults.standard.set(data, forKey: "keyMapping")
            UserDefaults.standard.synchronize()
        }
    }

    func mapKey(_ keyboardKey: SaturnKey, toControl button: KeyMapMappableButton) {
        keyMapping[button] = keyboardKey
        print( "mapKey: \(button) to \(keyboardKey)" )
    }

    func unmapKey(_ saturnKey: SaturnKey) {
        keyMapping = keyMapping.filter { $1 != saturnKey }
    }

    //@objc func getMappedKey(forControl button: KeyMapMappableButton) -> SaturnKey? {
    //    return keyMapping[button]
    //}

    @objc func getMappedKey(forControl button: KeyMapMappableButton) -> NSNumber? {
        guard let key = keyMapping[button] else { return nil }
        return NSNumber(value: key.rawValue)
    }
    
    //;.@objc func getControls(forMappedKey keyboardKey: SaturnKey) -> [KeyMapMappableButton] {
    //    return keyMapping.filter { $1 == keyboardKey }.map { $0.key }
   // }
    
    @objc func getControls(forMappedKey keyboardKey: SaturnKey) -> [NSNumber] {
        return keyMapping.filter { $1 == keyboardKey }.map { NSNumber(value: $0.key.rawValue) }
    }
    

    static func controlToDisplayName(_ button: KeyMapMappableButton) -> String {
        switch button {
        case .MFI_BUTTON_HOME: return "H"
        case .MFI_BUTTON_OPTION: return "O"
        case .MFI_BUTTON_MENU: return "M"
        case .MFI_BUTTON_A: return "A"
        case .MFI_BUTTON_B: return "B"
        case .MFI_BUTTON_X: return "X"
        case .MFI_BUTTON_Y: return "Y"
        case .MFI_BUTTON_LS: return "LS"
        case .MFI_BUTTON_LT: return "LT"
        case .MFI_BUTTON_RS: return "RS"
        case .MFI_BUTTON_RT: return "RT"
        case .MFI_DPAD_UP: return "⬆️"
        case .MFI_DPAD_DOWN: return "⬇️"
        case .MFI_DPAD_LEFT: return "⬅️"
        case .MFI_DPAD_RIGHT: return "➡️"
        default: return "?"
        }
    }
}

