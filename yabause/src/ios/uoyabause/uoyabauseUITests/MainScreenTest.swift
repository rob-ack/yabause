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
/*
        // Wait 5 seconds
        let expectation = self.expectation(description: "Waiting for 5 seconds")

        // 5秒後に期待を満たす
        DispatchQueue.main.asyncAfter(deadline: .now() + 5) {
            expectation.fulfill()
        }

        // 期待が満たされるまで5秒待機
        wait(for: [expectation], timeout: 5.1)
*/
        snapshot("01MainScreen")
        
        // Wait for the "Settings" button to appear (timeout after 10 seconds)
        let settingsButton = app!.buttons["settingButton"]
        XCTAssertTrue(settingsButton.waitForExistence(timeout: 10), "Settings button did not appear")
        
        // Tap the button
        settingsButton.tap()
        
        snapshot("02NextScreen")
  }
}
