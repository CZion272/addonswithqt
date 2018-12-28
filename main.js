var binary = require('node-pre-gyp');
var path = require('path');
var binding_path = binary.find(path.resolve(path.join(__dirname,'./package.json')));
var binding = require(binding_path);

exports.SetMaxThread = function(threadNumber) {
  return binding.setMaxThread(threadNumber);
}

class CReadImage{
    constructor(imagePath, savepath){
        this.obj = new binding.CImageReader(imagePath, savepath);
    }
	
	setDefaultColorList(colorlist){
		this.obj.setDefaultColorList(colorlist);
	}
	
	setDefaultImageSize(width, height){
		this.obj.setDefaultImageSize(colorlist);
	}
	
	setDefaultMD5(md5){
		this.obj.setDefaultImageSize(md5);
	}
	
	setPreviewSize(width, height){
		this.obj.setPreviewSize(width, height);
	}
	
	readFile(funcation){
		this.obj.readFile(funcation);
	}
	
	cancel(){
		this.obj.cancel();
	}
	
	MD5(){
		return this.obj.MD5();
	}
	
	compareColor(color){
		return this.obj.compareColor(color);
	}
	
	colorCount(){
		return this.obj.colorCount();
	}
	
	colorAt(index){
		return this.obj.colorAt(index);
	}
	
	imageWidth(){
		return this.obj.imageWidth();
	}
	
	imageHeight(){
		return this.obj.imageHeight();
	}
}
window.CReadImage = CReadImage;
