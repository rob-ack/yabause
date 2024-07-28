//
//  GameInfo.swift
//  YabaSnashiro
//
//  Created by Shinya Miyamoto on 2024/07/14.
//  Copyright © 2024 devMiyax. All rights reserved.
//

import Foundation

struct GameInfo {
    var filePath: String?
    var isoFilePath: String?
    var makerId: String?
    var productNumber: String?
    var version: String?
    var releaseDate: String?
    var area: String?
    var inputDevice: String?
    var deviceInformation: String?
    var gameTitle: String?
    var displayName: String?
    var imageUrl: String?
}

func getGameInfoFromBuf(filePath: String?, header: Data) -> GameInfo? {
    guard let filePath = filePath, !header.isEmpty else { return nil }
    
    let checkStr = "SEGA ".data(using: .utf8)!
    guard let startIndex = header.range(of: checkStr)?.lowerBound else { return nil }
    
    var gameInfo = GameInfo()
    gameInfo.filePath = filePath
    gameInfo.isoFilePath = filePath.uppercased()
    
    let charset = String.Encoding.shiftJIS // MS932に相当
    
    if let makerIdData = header.subdata(in: startIndex+0x10..<startIndex+0x20).string(using: charset) {
        gameInfo.makerId = makerIdData.trimmingCharacters(in: .whitespaces)
    }
    
    if let productNumberData = header.subdata(in: startIndex+0x20..<startIndex+0x2A).string(using: charset) {
        gameInfo.productNumber = productNumberData.trimmingCharacters(in: .whitespaces)
        if( gameInfo.productNumber != nil ){
            if let path = Bundle.main.path(forResource: "secrets", ofType: "plist"),
               let dict = NSDictionary(contentsOfFile: path) as? [String: AnyObject] {
                gameInfo.imageUrl = "https://d3edktb2n8l35b.cloudfront.net/BOXART/"+gameInfo.productNumber!+".PNG?" + ((dict["cloudfront"] as? String)!)
            }
        }
    }
    
    if let versionData = header.subdata(in: startIndex+0x2A..<startIndex+0x3A).string(using: charset) {
        gameInfo.version = versionData.trimmingCharacters(in: .whitespaces)
    }
    
    if let releaseDateData = header.subdata(in: startIndex+0x30..<startIndex+0x38).string(using: charset) {
        gameInfo.releaseDate = releaseDateData.trimmingCharacters(in: .whitespaces)
    }
    
    if let areaData = header.subdata(in: startIndex+0x40..<startIndex+0x4A).string(using: charset) {
        gameInfo.area = areaData.trimmingCharacters(in: .whitespaces)
    }
    
    if let inputDeviceData = header.subdata(in: startIndex+0x50..<startIndex+0x60).string(using: charset) {
        gameInfo.inputDevice = inputDeviceData.trimmingCharacters(in: .whitespaces)
    }
    
    if let deviceInformationData = header.subdata(in: startIndex+0x38..<startIndex+0x40).string(using: charset) {
        gameInfo.deviceInformation = deviceInformationData.trimmingCharacters(in: .whitespaces)
    }
    
    if let gameTitleData = header.subdata(in: startIndex+0x60..<startIndex+0xD0).string(using: charset) {
        gameInfo.gameTitle = gameTitleData.trimmingCharacters(in: .whitespaces)
    }
    
    let gameTitle = gameInfo.gameTitle ?? ""
    let titles = gameTitle.components(separatedBy: "U:")

    if titles.count >= 2 {
        let japaneseTitle = titles[0].replacingOccurrences(of: "J:", with: "").trimmingCharacters(in: .whitespaces)
        let englishTitle = titles[1].trimmingCharacters(in: .whitespaces)
        
        if Locale.current.language.languageCode?.identifier == "ja" {
            //cell.titleLabel.text = japaneseTitle
            gameInfo.displayName = japaneseTitle
        } else {
            //cell.titleLabel.text = englishTitle
            gameInfo.displayName = englishTitle
        }
    } else {
        gameInfo.displayName = gameInfo.gameTitle
    }

    if( gameInfo.deviceInformation != "CD-1/1" ){
        gameInfo.displayName = (gameInfo.displayName  ?? "") + " " + (gameInfo.deviceInformation ?? "")
    }

    
    
    return gameInfo
}

extension Data {
    func string(using encoding: String.Encoding) -> String? {
        return String(data: self, encoding: encoding)
    }
}
