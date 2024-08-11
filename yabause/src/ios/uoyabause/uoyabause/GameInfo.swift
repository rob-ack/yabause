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

enum GameInfoError: Error {
    case isoFileNotFound(message: String)
}

func genGameInfoFromIso(_ filePath: String?) -> GameInfo? {
    guard let filePath = filePath else { return nil }
    
    do {
        let fileURL = URL(fileURLWithPath: filePath)
        let data = try Data(contentsOf: fileURL)
        let buff = data.prefix(0xFF)
        
        return getGameInfoFromBuf(filePath: filePath, header: buff)
    } catch {
        print(error)
        return nil
    }
}

func genGameInfoFromMDS(filePath: String?) throws -> GameInfo? {
    guard let filePath = filePath else { return nil }
    let isoFileName = filePath.replacingOccurrences(of: ".mds", with: ".mdf")
    
    let fileManager = FileManager.default
    if !fileManager.fileExists(atPath: isoFileName) {
        // ISOファイルが存在しない場合に例外を発行
        throw GameInfoError.isoFileNotFound(message: "ISO file \(isoFileName) does not exist.")
    }

    
    guard var tmp = genGameInfoFromIso(isoFileName) else { return nil }
    tmp.filePath = filePath
    return tmp
}


func genGameInfoFromCCD(filePath: String?) throws -> GameInfo? {
    guard let filePath = filePath else { return nil }
    let isoFileName = filePath.replacingOccurrences(of: ".ccd", with: ".img")
    
    let fileManager = FileManager.default
    if !fileManager.fileExists(atPath: isoFileName) {
        // ISOファイルが存在しない場合に例外を発行
        throw GameInfoError.isoFileNotFound(message: "ISO file \(isoFileName) does not exist.")
    }
    
    
    guard var tmp = genGameInfoFromIso(isoFileName) else { return nil }
    tmp.filePath = filePath
    return tmp
}


func genGameInfoFromCUE(filePath: String?) throws -> GameInfo? {
    guard let filePath = filePath else { return nil }
    let file = URL(fileURLWithPath: filePath)
    let dirName = file.deletingLastPathComponent().path
    var isoFileName = ""
    
    do {
        let fileContent = try String(contentsOf: file, encoding: .utf8)
        let lines = fileContent.split(separator: "\n")
        
        for line in lines {
            let pattern = "FILE \"(.*)\""
            if let range = line.range(of: pattern, options: .regularExpression) {
                isoFileName = String(line[range])
                isoFileName = isoFileName.replacingOccurrences(of: "FILE \"", with: "").replacingOccurrences(of: "\"", with: "")
                break
            }
        }
        
        if isoFileName.isEmpty {
           throw GameInfoError.isoFileNotFound(message: "ISO file does not exist.")
        }
    } catch {
        print(error)
        return nil
    }
    
    if !dirName.isEmpty {
        isoFileName = (dirName as NSString).appendingPathComponent(isoFileName)
        
        let fileManager = FileManager.default
        if !fileManager.fileExists(atPath: isoFileName) {
            // ISOファイルが存在しない場合に例外を発行
            throw GameInfoError.isoFileNotFound(message: "ISO file \(isoFileName) does not exist.")
        }
    }
    
    guard var tmp = genGameInfoFromIso(isoFileName) else { return nil }
    tmp.filePath = filePath
    return tmp
}
        
        
func getGameInfoFromBuf(filePath: String?, header: Data) -> GameInfo? {
    guard let filePath = filePath, !header.isEmpty else { return nil }
    
    let checkStr = "SEGA ".data(using: .utf8)!
    guard let startIndex = header.range(of: checkStr)?.lowerBound else { return nil }
    
    var gameInfo = GameInfo()
    gameInfo.filePath = filePath
    
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
