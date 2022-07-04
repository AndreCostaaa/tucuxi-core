//
// Created by fiona on 6/23/22.
//

#ifndef TUCUXI_SIGNATUREVALIDATOR_H
#define TUCUXI_SIGNATUREVALIDATOR_H

#include <string>
#include <iostream>
#include <botan/pk_keys.h>
#include <botan/data_src.h>
#include <botan/pem.h>
#include <botan/x509cert.h>
#include <botan/auto_rng.h>
#include <botan/pubkey.h>
#include <botan/hex.h>
#include <botan/base64.h>
#include <botan/x509path.h>
#include "signature.h"
#include "signer.h"

namespace Tucuxi {
namespace Common {

class SignValidator
{
private:
    /// \brief Load a certificate from a certificate in a PEM encoded string
    /// \param certPem The PEM encoded certificate
    /// \return The certificate object
    static Botan::X509_Certificate loadCert(std::string certPem);

    /// \brief Load the public key of a certificate
    /// \param cert The certificate
    /// \return The certificate public key
    static Botan::Public_Key* loadPublicKey(Botan::X509_Certificate& cert);

    /// \brief Verify a signature
    /// \param publicKey The public key
    /// \param base64Signature The signature
    /// \param signedData The data that has been signed
    /// \return True if the signature is valid
    static bool verifySignature(Botan::Public_Key* publicKey, std::string base64Signature, std::string signedData);

    /// \brief Verify the certificate chain
    /// \param userCert The user certificate
    /// \param signingCert The intermediate certificate
    /// \return True if the certificate chain is valid
    static bool verifyChain(Botan::X509_Certificate& userCert, Botan::X509_Certificate& signingCert);

public:
    /// \brief Validate a signature
    /// \param signature The signature to validate
    /// \return True if the signature and certificate chain is valid
    static bool validateSignature(Signature& signature);

    /// \brief Load the certificate owner information
    /// \param certPem The PEM encoded certificate
    /// \return The signer object containing the signer information
    static Signer loadSigner(std::string certPem);
};

}
}

#endif //TUCUXI_SIGNATUREVALIDATOR_H
