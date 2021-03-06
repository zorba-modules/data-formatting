/*
 * Copyright 2006-2008 The FLWOR Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <list>

#include <zorba/diagnostic_list.h>
#include <zorba/empty_sequence.h>
#include <zorba/external_module.h>
#include <zorba/function.h>
#include <zorba/item_factory.h>
#include <zorba/serializer.h>
#include <zorba/singleton_item_sequence.h>
#include <zorba/user_exception.h>
#include <zorba/user_exception.h>
#include <zorba/util/base64_util.h>
#include <zorba/vector_item_sequence.h>
#include <zorba/zorba.h>

#include "JavaVMSingleton.h"

#define XSL_MODULE_NAMESPACE "http://zorba.io/modules/xsl-fo"

class JavaException {
};

#define CHECK_EXCEPTION(env)  if ((lException = env->ExceptionOccurred())) throw JavaException()

namespace zorba { namespace xslfo {
 
class GeneratePDFFunction : public ContextualExternalFunction {
  private:
    const ExternalModule* theModule;
    ItemFactory* theFactory;
  public:
    GeneratePDFFunction(const ExternalModule* aModule) :
      theModule(aModule), theFactory(Zorba::getInstance(0)->getItemFactory()) {}
    ~GeneratePDFFunction() {}

  public:
    virtual String getURI() const { return theModule->getURI(); }

    virtual String getLocalName() const { return "generator-impl"; }

    virtual ItemSequence_t 
    evaluate(const ExternalFunction::Arguments_t& args,
             const zorba::StaticContext*,
             const zorba::DynamicContext*) const;
};

class XSLFOModule : public ExternalModule {
  private:
    ExternalFunction* generatePDF;
  public:
    XSLFOModule() :
      generatePDF(new GeneratePDFFunction(this))
  {}
    ~XSLFOModule() {
      delete generatePDF;
    }

    virtual String getURI() const { return XSL_MODULE_NAMESPACE; }

    virtual ExternalFunction* getExternalFunction(const String& localName);

    virtual void destroy() {
      delete this;
    }
};

ExternalFunction* XSLFOModule::getExternalFunction(const String& localName) {
  if (localName == "generator-impl") {
    return generatePDF;
  } 
  return 0;
}

ItemSequence_t
GeneratePDFFunction::evaluate(const ExternalFunction::Arguments_t& args,
                              const zorba::StaticContext* aStaticContext,
                              const zorba::DynamicContext* aDynamincContext) const
{
  Iterator_t lIter = args[0]->getIterator();
  lIter->open();
  Item outputFormat;
  lIter->next(outputFormat);
  lIter->close();
  jthrowable lException = 0;
  static JNIEnv* env;
  try {
    env = zorba::jvm::JavaVMSingleton::getInstance(aStaticContext)->getEnv();
    jstring outFotmatString = env->NewStringUTF(outputFormat.getStringValue().c_str());
    // Local variables
    std::ostringstream os;
    Zorba_SerializerOptions_t lOptions;
    Serializer_t lSerializer = Serializer::createSerializer(lOptions);
    jclass fopFactoryClass;
    jobject fopFactory;
    jmethodID fopFactoryNewInstance;
    jclass byteArrayOutputStreamClass;
    jobject byteArrayOutputStream;
    jobject fop;
    jmethodID newFop;
    jclass transformerFactoryClass;
    jobject transformerFactory;
    jobject transormer;
    jclass stringReaderClass;
    jobject stringReader;
    jstring xmlUTF;
    const char* xml;
    std::string xmlString;
    jclass streamSourceClass;
    jobject streamSource;
    jobject defaultHandler;
    jclass saxResultClass;
    jobject saxResult;
    jboolean isCopy;
    jbyteArray res;
    Item base64;
    String resStore;
    jsize dataSize;
    jbyte* dataElements;

    Item item;
    lIter = args[1]->getIterator();
    lIter->open();
    lIter->next(item);
    lIter->close();
    // Searialize Item
    SingletonItemSequence lSequence(item);
    lSerializer->serialize(&lSequence, os);
    xmlString = os.str();
    xml = xmlString.c_str();

    // Create an OutputStream
    byteArrayOutputStreamClass = env->FindClass("java/io/ByteArrayOutputStream");
    CHECK_EXCEPTION(env);
    byteArrayOutputStream = env->NewObject(byteArrayOutputStreamClass,
        env->GetMethodID(byteArrayOutputStreamClass, "<init>", "()V"));
    CHECK_EXCEPTION(env);

    // Create a FopFactory instance
    fopFactoryClass = env->FindClass("org/apache/fop/apps/FopFactory");
    CHECK_EXCEPTION(env);
    fopFactoryNewInstance = env->GetStaticMethodID(fopFactoryClass, "newInstance", "()Lorg/apache/fop/apps/FopFactory;");
    CHECK_EXCEPTION(env);
    fopFactory = env->CallStaticObjectMethod(fopFactoryClass, fopFactoryNewInstance);
    CHECK_EXCEPTION(env);

    // Create the Fop
    newFop = env->GetMethodID(fopFactoryClass, "newFop", "(Ljava/lang/String;Ljava/io/OutputStream;)Lorg/apache/fop/apps/Fop;");
    CHECK_EXCEPTION(env);
    fop = env->CallObjectMethod(fopFactory,
        newFop,
        outFotmatString, byteArrayOutputStream);
    CHECK_EXCEPTION(env);

    // Create the Transformer
    transformerFactoryClass = env->FindClass("javax/xml/transform/TransformerFactory");
    CHECK_EXCEPTION(env);
    transformerFactory = env->CallStaticObjectMethod(transformerFactoryClass,
        env->GetStaticMethodID(transformerFactoryClass, "newInstance", "()Ljavax/xml/transform/TransformerFactory;"));
    CHECK_EXCEPTION(env);
    transormer = env->CallObjectMethod(transformerFactory,
        env->GetMethodID(transformerFactoryClass, "newTransformer", "()Ljavax/xml/transform/Transformer;"));
    CHECK_EXCEPTION(env);

    // Create Source
    xmlUTF = env->NewStringUTF(xml);
    stringReaderClass = env->FindClass("java/io/StringReader");
    CHECK_EXCEPTION(env);
    stringReader = env->NewObject(stringReaderClass,
        env->GetMethodID(stringReaderClass, "<init>", "(Ljava/lang/String;)V"), xmlUTF);
    CHECK_EXCEPTION(env);
    streamSourceClass = env->FindClass("javax/xml/transform/stream/StreamSource");
    CHECK_EXCEPTION(env);
    streamSource = env->NewObject(streamSourceClass,
        env->GetMethodID(streamSourceClass, "<init>", "(Ljava/io/Reader;)V"), stringReader);
    CHECK_EXCEPTION(env);

    // Create the SAXResult 
    defaultHandler = env->CallObjectMethod(fop,
        env->GetMethodID(env->FindClass("org/apache/fop/apps/Fop"), "getDefaultHandler",
          "()Lorg/xml/sax/helpers/DefaultHandler;"));
    CHECK_EXCEPTION(env);
    saxResultClass = env->FindClass("javax/xml/transform/sax/SAXResult");
    CHECK_EXCEPTION(env);
    saxResult = env->NewObject(saxResultClass,
        env->GetMethodID(saxResultClass, "<init>", "(Lorg/xml/sax/ContentHandler;)V"),
        defaultHandler);
    CHECK_EXCEPTION(env);

    // Transform
    env->CallObjectMethod(transormer,
        env->GetMethodID(env->FindClass("javax/xml/transform/Transformer"), 
          "transform", 
          "(Ljavax/xml/transform/Source;Ljavax/xml/transform/Result;)V"),
        streamSource, saxResult);
    CHECK_EXCEPTION(env);

    // Close outputstream
    env->CallObjectMethod(byteArrayOutputStream,
        env->GetMethodID(env->FindClass("java/io/OutputStream"),
          "close", "()V"));
    CHECK_EXCEPTION(env);
    saxResultClass = env->FindClass("javax/xml/transform/sax/SAXResult");
    CHECK_EXCEPTION(env);

    // Get the byte array
    res = (jbyteArray) env->CallObjectMethod(byteArrayOutputStream,
        env->GetMethodID(byteArrayOutputStreamClass, "toByteArray", "()[B"));
    CHECK_EXCEPTION(env);

    // Create the result
    dataSize = env->GetArrayLength(res);
    dataElements = env->GetByteArrayElements(res, &isCopy);

    std::string lBinaryString((const char*) dataElements, dataSize);
    std::stringstream lStream(lBinaryString);
    String base64S;
    base64::encode(lStream, &base64S);
    Item lRes( theFactory->createBase64Binary(base64S.data(), base64S.size(), true) );
    return ItemSequence_t(new SingletonItemSequence(lRes));
  } catch (zorba::jvm::VMOpenException&) {
    Item lQName = theFactory->createQName("http://zorba.io/modules/xsl-fo",
        "JVM-NOT-STARTED");
    throw USER_EXCEPTION(lQName, "Could not start the Java VM (is the classpath set?)");
  } catch (JavaException&) {
    jclass stringWriterClass = env->FindClass("java/io/StringWriter");
    jclass printWriterClass = env->FindClass("java/io/PrintWriter");
    jclass throwableClass = env->FindClass("java/lang/Throwable");
    jobject stringWriter = env->NewObject(
        stringWriterClass,
        env->GetMethodID(stringWriterClass, "<init>", "()V"));
    jobject printWriter = env->NewObject(
        printWriterClass, env->GetMethodID(printWriterClass, "<init>", "(Ljava/io/Writer;)V"), stringWriter);
    env->CallObjectMethod(lException, env->GetMethodID(throwableClass, "printStackTrace", "(Ljava/io/PrintWriter;)V"), printWriter);
    //env->CallObjectMethod(printWriter, env->GetMethodID(printWriterClass, "flush", "()V"));
    jmethodID toStringMethod = env->GetMethodID(stringWriterClass, "toString", "()Ljava/lang/String;");
    jobject errorMessageObj = env->CallObjectMethod(
        stringWriter, toStringMethod);
    jstring errorMessage = (jstring) errorMessageObj;
    const char *errMsg = env->GetStringUTFChars(errorMessage, 0);
    std::stringstream s;
    s << "A Java Exception was thrown:" << std::endl << errMsg;
    env->ReleaseStringUTFChars(errorMessage, errMsg);
    std::string err("");
    err += s.str();
    env->ExceptionClear();
    Item lQName = theFactory->createQName("http://zorba.io/modules/xsl-fo",
        "JAVA-EXCEPTION");
    throw USER_EXCEPTION(lQName, err);
  }
  return ItemSequence_t(new EmptySequence());
}

}}; // namespace zorba, xslfo

#ifdef WIN32
#  define DLL_EXPORT __declspec(dllexport)
#else
#  define DLL_EXPORT __attribute__ ((visibility("default")))
#endif

extern "C" DLL_EXPORT zorba::ExternalModule* createModule() {
  return new zorba::xslfo::XSLFOModule();
}
