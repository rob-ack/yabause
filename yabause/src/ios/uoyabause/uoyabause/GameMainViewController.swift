//
//  GameMainViewController.swift
//  YabaSnashiro
//
//  Created by Shinya Miyamoto on 2024/07/21.
//  Copyright © 2024 devMiyax. All rights reserved.
//

import Foundation
import UIKit

class GameMainViewController : UIViewController {
    
    enum MenuState {
        case opened
        case closed
    }
    
    private var menuState : MenuState = .closed
    
    var selectedFile = ""
    private var gameVC: GameViewController?
    let menuVC = MenuViewController()

     override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
         if let gameVC = segue.destination as? GameViewController {
             self.gameVC = gameVC
             gameVC.selectedFile = self.selectedFile
             self.gameVC?.gdelegate = self
         }
     }

    override func viewDidLoad() {
        super.viewDidLoad()
        menuVC.delegate = self
        view.backgroundColor = .black
        addChild(menuVC)
        view.addSubview(menuVC.view)
        menuVC.didMove(toParent: self)
        view.sendSubviewToBack(menuVC.view)
    }
    
    override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
        // landscapeフラグに応じて画面の向きを設定
        let ud = UserDefaults.standard
        let landscape = ud.bool(forKey: "landscape")
        
        if landscape {
            return .landscape
        } else {
            return .all
        }
    }
    
}

extension GameMainViewController: GameViewControllerDelegate {
    func didTapMenuButton() {
        // Animate the menu
        print("tap the menu")
        toggleMenu( completion: nil )
    }
    
    func toggleMenu( completion: (() -> Void)? ){
        switch menuState {
        case .closed:
            
            self.gameVC?.isPaused = true
            
            // open it
            UIView.animate(withDuration: 0.5, delay: 0, usingSpringWithDamping: 0.8, initialSpringVelocity: 0, options: .curveEaseInOut) {
                
                self.gameVC?.view.frame.origin.x = 240 //a(self.gameVC?.view.frame.size.width ?? 100) - 100
                
            }completion:{ [weak self] done in
                if done {
                    self?.menuState = .opened
                    completion?()
                }
            }
            
        case .opened:
            
            self.gameVC?.isPaused = false
            
            // close it
            UIView.animate(withDuration: 0.5, delay: 0, usingSpringWithDamping: 0.8, initialSpringVelocity: 0, options: .curveEaseInOut) {
                
                self.gameVC?.view.frame.origin.x = 0
                
            }completion:{ [weak self] done in
                if done {
                    self?.menuState = .closed
                    DispatchQueue.main.async {
                        completion?()
                    }
                }
            }
        }
    }
}


extension GameMainViewController: MenuViewControllerDelegate {
    func didSelect(menuItem: MenuViewController.MenuOptions) {
        toggleMenu { [weak self] in
            switch menuItem {
            case .reset:
                self?.gameVC?.reset()
                break
            case .changeDisk:
                self?.gameVC?.presentFileSelectViewController()
                break
            case .saveState:
                self?.gameVC?.saveState()
                break
            case .loadState:
                self?.gameVC?.loadState()
                break
            //case .analogMode:
            //    break
            case .controllerSetting:
                self?.gameVC?.toggleControllSetting()
                break
            }
        }
    }
    
}
