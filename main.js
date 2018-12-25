var binary = require('node-pre-gyp');
var path = require('path');
var binding_path = binary.find(path.resolve(path.join(__dirname,'./package.json')));
var binding = require(binding_path);


class CReadImage{
    constructor(imagePath, savepath, ratio){
        this.obj = new binding.CImageReader(imagePath, savepath, ratio);
    }
	
	readFile(funcation){
		this.obj.readFile(funcation);
	}
	
	MD5(){
		return this.obj.MD5();
	}
	
	compareColor(color){
		return this.obj.checkColor(color);
	}
	
	colorCount(){
		return this.obj.colorCount();
	}
	
	colorAt(index){
		return this.obj.colorAt(index);
	}
	
	imageWidth(){
		return this.obj.ImageWidth();
	}
	
	imageHeight(){
		return this.obj.ImageHeight();
	}
}
window.CReadImage = CReadImage;
