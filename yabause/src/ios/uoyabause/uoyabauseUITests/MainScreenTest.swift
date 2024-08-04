//
//  MainScreenTest.swift
//  YabaSnashiroUITests
//
//  Created by devMiyax on 2024/08/04.
//  Copyright © 2024 devMiyax. All rights reserved.
//

import Foundation
import XCTest

final class uoyabauseUITests: XCTestCase {
  
    var app:XCUIApplication?
    
    @MainActor override func setUp() {
        super.setUp()
        continueAfterFailure = false
        self.app = XCUIApplication()
        setupSnapshot(self.app!)
        self.app!.launch()
    }
    
    @MainActor
    func testScreenShot1() {
        snapshot("01MainScreen")
        self.app!.buttons["Settings"].tap()
        snapshot("02NextScreen")
  }
}
