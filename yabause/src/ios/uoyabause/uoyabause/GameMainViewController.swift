//
//  GameMainViewController.swift
//  YabaSnashiro
//
//  Created by Shinya Miyamoto on 2024/07/21.
//  Copyright © 2024 devMiyax. All rights reserved.
//

import Foundation
import UIKit



class GameMainViewController: UIViewController
{
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
    
    override var prefersHomeIndicatorAutoHidden: Bool {
        return true
    }
    
    //@available(iOS 11, *)
    override var childForHomeIndicatorAutoHidden: UIViewController? {
        return nil
    }
}

extension GameMainViewController: GameViewControllerDelegate {
   
    func didTapMenuButton() {
        // Animate the menu
        // print("tap the menu")
        
        if( menuState == .opened ){
            self.gameVC?.isPaused = false
        }
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

            var doNotPause = false
            switch menuItem {
            case .exit:
                // アラートコントローラの作成
                let alertController = UIAlertController(title: "", message: "Are you sure you want to exit?", preferredStyle: .alert)

                // "Yes"ボタンの追加
                let yesAction = UIAlertAction(title: "Yes", style: .default) { action in
                    exit(0)
                }
                alertController.addAction(yesAction)

                // "No"ボタンの追加
                let noAction = UIAlertAction(title: "No", style: .default) { action in
                    self?.gameVC?.isPaused = false
                }
                alertController.addAction(noAction)

                self?.present(alertController, animated: true, completion: nil)
                doNotPause = true
                break
            case .reset:
                self?.gameVC?.reset()
                break
            case .changeDisk:
                self?.gameVC?.presentFileSelectViewController()
                doNotPause = true
                break
            case .saveState:
                self?.gameVC?.saveState()
                break
            case .loadState:
                self?.gameVC?.loadState()
                break
            case .analogMode:
                break
            case .controllerSetting:
                self?.gameVC?.toggleControllSetting()
                break
            }

            if doNotPause == false{
                self?.gameVC?.isPaused = false
            }
        }
    }
    
    func didChangeAnalogMode(to: Bool){
        
        toggleMenu { [weak self] in
            let plist = SettingsViewController.getSettingPlist();
            plist.setObject(to, forKey: "analog mode" as NSCopying)
            plist.write(toFile: SettingsViewController.getSettingFilname(), atomically: true)
            self?.gameVC?.setAnalogMode(to: to)
            self?.gameVC?.isPaused = false
        }
    }
    
}
