//
//  MainScreenController.swift
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/09/04.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

import Foundation
import UIKit
import UniformTypeIdentifiers

class MainScreenController :UIViewController, UIDocumentPickerDelegate  {
    
    var activityIndicator: UIActivityIndicatorView!
    var blurEffectView: UIVisualEffectView!
    var selected_file_path: String = ""
    @IBOutlet weak var settingButton: UIButton!

    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // Blur Effect Viewの設定
        let blurEffect = UIBlurEffect(style: .dark)
        blurEffectView = UIVisualEffectView(effect: blurEffect)
        blurEffectView.frame = self.view.bounds
        blurEffectView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        blurEffectView.isHidden = true
        self.view.addSubview(blurEffectView)
        
        // Activity Indicatorの設定
        activityIndicator = UIActivityIndicatorView(style: .large)
        activityIndicator.center = self.view.center
        activityIndicator.hidesWhenStopped = true
        self.view.addSubview(activityIndicator)
        
        // Activity Indicatorをビュー階層の一番上に持ってくる
        self.view.bringSubviewToFront(activityIndicator)
        #if FREE_VERSION
        self.navigationItem.title = "Yaba Sanshiro 2 Lite"
        #endif
        
       settingButton.accessibilityIdentifier = "settingButton"
        
    }

   
    @IBAction func onAddFile(_ sender: Any) {
        
        for child in self.children {
            if let fc = child as? FileSelectController {
                if fc.checkLimitation() == false {
                    return
                }
            }
        }
        
        var documentPicker: UIDocumentPickerViewController!
            // iOS 14 & later
            let supportedTypes: [UTType] = [
                UTType(filenameExtension: "bin")!,
                UTType(filenameExtension: "cue")!,
                UTType(filenameExtension: "chd")!,
                UTType(filenameExtension: "ccd")!,
                UTType(filenameExtension: "img")!,
                UTType(filenameExtension: "mds")!,
                UTType(filenameExtension: "mdf")!,
            ]
            
        documentPicker = UIDocumentPickerViewController(forOpeningContentTypes: supportedTypes)
        documentPicker.delegate = self
        documentPicker.allowsMultipleSelection = true
        self.present(documentPicker, animated:true, completion: nil)
    }
    
    func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]){
        
        self.view.bringSubviewToFront(activityIndicator)
        blurEffectView.isHidden = false
        activityIndicator.startAnimating()
        
        DispatchQueue.global(qos: .userInitiated).async {
                // 選択されたファイルのURLを処理
            for url in urls {
                // ここで各ファイルのURLを処理します
                print("Selected file URL: \(url)")
                // 例: ファイルを解凍して処理する
                self.processFile(at: url)
            }
            
            DispatchQueue.main.async {
                self.children.forEach{
                    let fc = $0 as? FileSelectController
                    if fc != nil {
                        fc?.updateDoc()
                    }
                }
                self.blurEffectView.isHidden = true
                self.activityIndicator.stopAnimating()
            }
        }
        
    }
    
    func processFile(at fileURL: URL) {
        print(fileURL)

        guard fileURL.startAccessingSecurityScopedResource() else {
            // エラー処理
            return
        }
        
        var documentsUrl = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        
        
        let theFileName = fileURL.lastPathComponent
            
        if theFileName.lowercased().contains(".cue") ||
            theFileName.lowercased().contains(".bin") ||
            theFileName.lowercased().contains(".chd") ||
            theFileName.lowercased().contains(".ccd") ||
            theFileName.lowercased().contains(".img") ||
            theFileName.lowercased().contains(".mdf") ||
            theFileName.lowercased().contains(".mds")
        {
            documentsUrl.appendPathComponent(theFileName)
            
            if( documentsUrl != fileURL ){
                
                let fileManager = FileManager.default
                do {
                    if fileManager.fileExists(atPath: documentsUrl.path) {
                        try fileManager.removeItem(at: documentsUrl)
                    }
                    try fileManager.copyItem(at: fileURL, to: documentsUrl)
                } catch let error as NSError {
                    print("Fail to copy \(error.localizedDescription)")
                    return
                }
            }
            
        } else{
            let alert: UIAlertController = UIAlertController(
                title: NSLocalizedString("Fail to open", comment: "Title for the alert when a file fails to open"),
                message: NSLocalizedString("You can select chd or bin or cue", comment: "Message indicating the supported file formats"),
                preferredStyle: UIAlertController.Style.alert
            )

            let defaultAction: UIAlertAction = UIAlertAction(
                title: NSLocalizedString("OK", comment: "Default action button title"),
                style: UIAlertAction.Style.default,
                handler: { (action: UIAlertAction!) -> Void in
                    print("OK")
                }
            )

            alert.addAction(defaultAction)
            present(alert, animated: true, completion: nil)
            return
        }
        
        fileURL.stopAccessingSecurityScopedResource()
        
                
    }
    
    
    
}
