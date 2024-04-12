//
//  MainScreenController.swift
//  uoyabause
//
//  Created by MiyamotoShinya on 2016/09/04.
//  Copyright © 2016年 devMiyax. All rights reserved.
//

import Foundation
import UIKit

class MainScreenController :UIViewController, UIDocumentPickerDelegate {

//    @IBOutlet weak var menu_setting: UIBarButtonItem!
    
    @IBAction func onAddFile(_ sender: Any) {
        let dv = UIDocumentPickerViewController(documentTypes:  ["public.item"], in: .import)
        dv.delegate = self
        //dv.allowsDocumentCreation = false
        //dv.allowsPickingMultipleItems = false
        self.present(dv, animated:true, completion: nil)
    }
    
    func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt documentURLs: [URL]){
        print(documentURLs[0])
        
        var documentsUrl = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        
        let theFileName = documentURLs[0].lastPathComponent
        
        if !theFileName.lowercased().contains(".chd") {
            let alert: UIAlertController = UIAlertController(title: "Fail to open", message: "You can open a CHD only", preferredStyle:  UIAlertController.Style.alert)
            
            let defaultAction: UIAlertAction = UIAlertAction(title: "OK", style: UIAlertAction.Style.default, handler:{
                (action: UIAlertAction!) -> Void in
                print("OK")
            })
            
            alert.addAction(defaultAction)
            
            present(alert, animated: true, completion: nil)
            return
        }

        documentsUrl.appendPathComponent(theFileName)
        let fileManager = FileManager.default
         do {
            try fileManager.copyItem(at: documentURLs[0], to: documentsUrl)
         } catch let error as NSError {
            NSLog("Fail to copy \(error.localizedDescription)")
         }
        
        self.children.forEach{
            //if ($0.isKind(of: FileSelectController)){
            let fc = $0 as? FileSelectController
            if fc != nil {
                fc?.updateDoc()
            }
            //}
        }
       
    }
    var val: Int = 0
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        let filePath = Bundle.main.path(forResource: "apikey", ofType: "plist")
        let plist = NSDictionary(contentsOfFile:filePath!)
        let value = plist?.value(forKey: "ADMOB_KEY") as! String
       
        
        val = 0
    }
    
    internal func onClickMyButton(_ sender: UIButton){
        let url = URL(string:UIApplication.openSettingsURLString)
        UIApplication.shared.openURL(url!)
    }
    
    
}
