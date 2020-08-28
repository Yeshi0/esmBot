#include <napi.h>
#include <list>
#include <Magick++.h>

using namespace std;
using namespace Magick;

class InvertWorker : public Napi::AsyncWorker {
 public:
  InvertWorker(Napi::Function& callback, string in_path, string type, int delay)
      : Napi::AsyncWorker(callback), in_path(in_path), type(type), delay(delay) {}
  ~InvertWorker() {}

  void Execute() {
    list <Image> frames;
    list <Image> coalesced;
    list <Image> result;
    readImages(&frames, in_path);
    coalesceImages(&coalesced, frames.begin(), frames.end());

    for_each(coalesced.begin(), coalesced.end(), negateImage(Magick::ChannelType(Magick::CompositeChannels ^ Magick::AlphaChannel)));
    for_each(coalesced.begin(), coalesced.end(), magickImage(type));

    optimizeImageLayers(&result, coalesced.begin(), coalesced.end());
    if (delay != 0) for_each(result.begin(), result.end(), animationDelayImage(delay));
    writeImages(result.begin(), result.end(), &blob);
  }

  void OnOK() {
    Callback().Call({Env().Undefined(), Napi::Buffer<char>::Copy(Env(), (char *)blob.data(), blob.length())});
  }

 private:
  string in_path, type;
  int delay;
  Blob blob;
};

Napi::Value Invert(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  Napi::Object obj = info[0].As<Napi::Object>();
  Napi::Function cb = info[1].As<Napi::Function>();
  string path = obj.Get("path").As<Napi::String>().Utf8Value();
  string type = obj.Get("type").As<Napi::String>().Utf8Value();
  int delay = obj.Get("delay").As<Napi::Number>().Int32Value();

  InvertWorker* invertWorker = new InvertWorker(cb, path, type, delay);
  invertWorker->Queue();
  return env.Undefined();
}