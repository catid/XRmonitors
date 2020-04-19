﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace XRmonitorsInstaller
{
    public partial class LegalPanel : UserControl
    {
        public LegalPanel()
        {
            InitializeComponent();

            // https://www.eulatemplate.com/download.php?lang=en&token=37JT4i8SuiyQJj2ftgqdskJ4r5AnUfL2#
            webEULA.DocumentText =
@"
<h2>End-User License Agreement (""Agreement"")</h2>

<p>Last updated: May 25, 2019</p>

<p>Please read this End-User License Agreement (""Agreement"") carefully before clicking the ""I Agree"" button, downloading or using XRmonitors (""Application"").</p>

<p>By clicking the ""I Agree"" button, downloading or using the Application, you are agreeing to be bound by the terms and conditions of this Agreement.</p>

<p>This Agreement is a legal agreement between you (either an individual or a single entity) and Augmented Perception Corporation and it governs your use of the Application made available to you by Augmented Perception Corporation.</p>

<p>If you do not agree to the terms of this Agreement, do not click on the ""I Agree"" button and do not download or use the Application.</p>

<p>The Application is licensed, not sold, to you by Augmented Perception Corporation for use strictly in accordance with the terms of this Agreement.</p>

<h2>License</h2>

<p>Augmented Perception Corporation grants you a revocable, non-exclusive, non-transferable, limited license to download, install and use the Application solely for your personal, non-commercial purposes strictly in accordance with the terms of this Agreement.</p>

<h2>Restrictions</h2>

<p>You agree not to, and you will not permit others to:</p>

<ul>
    
    <li>
        <p>license, sell, rent, lease, assign, distribute, transmit, host, outsource, disclose or otherwise commercially exploit the Application or make the Application available to any third party.</p>
    </li>
    <li>
        <p>copy or use the Application for any purpose other than as permitted under the above section 'License'.</p>
    </li>
    <li>
        <p>modify, make derivative works of, disassemble, decrypt, reverse compile or reverse engineer any part of the Application.</p>
    </li>
    <li>
        <p>remove, alter or obscure any proprietary notice (including any notice of copyright or trademark) of Augmented Perception Corporation or its affiliates, partners, suppliers or the licensors of the Application.</p>
    </li>
</ul>

<h2>Intellectual Property</h2>

<p>The Application, including without limitation all copyrights, patents, trademarks, trade secrets and other intellectual property rights are, and shall remain, the sole and exclusive property of Augmented Perception Corporation.</p>

<h2>Your Suggestions</h2>

<p>Any feedback, comments, ideas, improvements or suggestions (collectively, ""Suggestions"") provided by you to Augmented Perception Corporation with respect to the Application shall remain the sole and exclusive property of Augmented Perception Corporation.</p>

<p>Augmented Perception Corporation shall be free to use, copy, modify, publish, or redistribute the Suggestions for any purpose and in any way without any credit or any compensation to you.</p>

<h2>Modifications to Application</h2>

<p>Augmented Perception Corporation reserves the right to modify, suspend or discontinue, temporarily or permanently, the Application or any service to which it connects, with or without notice and without liability to you.</p>

<h2>Updates to Application</h2>

<p>Augmented Perception Corporation may from time to time provide enhancements or improvements to the features/functionality of the Application, which may include patches, bug fixes, updates, upgrades and other modifications (""Updates"").</p>

<p>Updates may modify or delete certain features and/or functionalities of the Application. You agree that Augmented Perception Corporation has no obligation to (i) provide any Updates, or (ii) continue to provide or enable any particular features and/or functionalities of the Application to you.</p>

<p>You further agree that all Updates will be (i) deemed to constitute an integral part of the Application, and (ii) subject to the terms and conditions of this Agreement.</p>

<h2>Third-Party Services</h2>

<p>The Application may display, include or make available third-party content (including data, information, applications and other products services) or provide links to third-party websites or services (""Third-Party Services"").</p>

<p>You acknowledge and agree that Augmented Perception Corporation shall not be responsible for any Third-Party Services, including their accuracy, completeness, timeliness, validity, copyright compliance, legality, decency, quality or any other aspect thereof. Augmented Perception Corporation does not assume and shall not have any liability or responsibility to you or any other person or entity for any Third-Party Services.</p>

<p>Third-Party Services and links thereto are provided solely as a convenience to you and you access and use them entirely at your own risk and subject to such third parties' terms and conditions.</p>

<h2>Privacy Policy</h2>

<p>Augmented Perception Corporation collects, stores, maintains, and shares information about you in accordance with its Privacy Policy, which is available at <a href=""https://www.xrmonitors.com/privacy-policy"">https://www.xrmonitors.com/privacy-policy</a>. By accepting this Agreement, you acknowledge that you hereby agree and consent to the terms and conditions of our Privacy Policy.</p>

<h2>Term and Termination</h2>

<p>This Agreement shall remain in effect until terminated by you or Augmented Perception Corporation.</p>

<p>Augmented Perception Corporation may, in its sole discretion, at any time and for any or no reason, suspend or terminate this Agreement with or without prior notice.</p>

<p>This Agreement will terminate immediately, without prior notice from Augmented Perception Corporation, in the event that you fail to comply with any provision of this Agreement. You may also terminate this Agreement by deleting the Application and all copies thereof from your mobile device or from your computer.</p>

<p>Upon termination of this Agreement, you shall cease all use of the Application and delete all copies of the Application from your mobile device or from your computer.</p>

<p>Termination of this Agreement will not limit any of Augmented Perception Corporation's rights or remedies at law or in equity in case of breach by you (during the term of this Agreement) of any of your obligations under the present Agreement.</p>

<h2>Indemnification</h2>

<p>You agree to indemnify and hold Augmented Perception Corporation and its parents, subsidiaries, affiliates, officers, employees, agents, partners and licensors (if any) harmless from any claim or demand, including reasonable attorneys' fees, due to or arising out of your: (a) use of the Application; (b) violation of this Agreement or any law or regulation; or (c) violation of any right of a third party.</p>

<h2>No Warranties</h2>

<p>The Application is provided to you ""AS IS"" and ""AS AVAILABLE"" and with all faults and defects without warranty of any kind. To the maximum extent permitted under applicable law, Augmented Perception Corporation, on its own behalf and on behalf of its affiliates and its and their respective licensors and service providers, expressly disclaims all warranties, whether express, implied, statutory or otherwise, with respect to the Application, including all implied warranties of merchantability, fitness for a particular purpose, title and non-infringement, and warranties that may arise out of course of dealing, course of performance, usage or trade practice. Without limitation to the foregoing, Augmented Perception Corporation provides no warranty or undertaking, and makes no representation of any kind that the Application will meet your requirements, achieve any intended results, be compatible or work with any other software, applications, systems or services, operate without interruption, meet any performance or reliability standards or be error free or that any errors or defects can or will be corrected.</p>

<p>Without limiting the foregoing, neither Augmented Perception Corporation nor any Augmented Perception Corporation's provider makes any representation or warranty of any kind, express or implied: (i) as to the operation or availability of the Application, or the information, content, and materials or products included thereon; (ii) that the Application will be uninterrupted or error-free; (iii) as to the accuracy, reliability, or currency of any information or content provided through the Application; or (iv) that the Application, its servers, the content, or e-mails sent from or on behalf of Augmented Perception Corporation are free of viruses, scripts, trojan horses, worms, malware, timebombs or other harmful components.</p>

<p>Some jurisdictions do not allow the exclusion of or limitations on implied warranties or the limitations on the applicable statutory rights of a consumer, so some or all of the above exclusions and limitations may not apply to you.</p>

<h2>Limitation of Liability</h2>

<p>Notwithstanding any damages that you might incur, the entire liability of Augmented Perception Corporation and any of its suppliers under any provision of this Agreement and your exclusive remedy for all of the foregoing shall be limited to the amount actually paid by you for the Application.</p>

<p>To the maximum extent permitted by applicable law, in no event shall Augmented Perception Corporation or its suppliers be liable for any special, incidental, indirect, or consequential damages whatsoever (including, but not limited to, damages for loss of profits, for loss of data or other information, for business interruption, for personal injury, for loss of privacy arising out of or in any way related to the use of or inability to use the Application, third-party software and/or third-party hardware used with the Application, or otherwise in connection with any provision of this Agreement), even if Augmented Perception Corporation or any supplier has been advised of the possibility of such damages and even if the remedy fails of its essential purpose.</p>

<p>Some states/jurisdictions do not allow the exclusion or limitation of incidental or consequential damages, so the above limitation or exclusion may not apply to you.</p>

<h2>Severability</h2>

<p>If any provision of this Agreement is held to be unenforceable or invalid, such provision will be changed and interpreted to accomplish the objectives of such provision to the greatest extent possible under applicable law and the remaining provisions will continue in full force and effect.</p>

<h2>Waiver</h2>

<p>Except as provided herein, the failure to exercise a right or to require performance of an obligation under this Agreement shall not effect a party's ability to exercise such right or require such performance at any time thereafter nor shall be the waiver of a breach constitute waiver of any subsequent breach.</p>

<h2>For U.S. Government End Users</h2>

<p>The Application and related documentation are ""Commercial Items"", as that term is defined under 48 C.F.R. §2.101, consisting of ""Commercial Computer Software"" and ""Commercial Computer Software Documentation"", as such terms are used under 48 C.F.R. §12.212 or 48 C.F.R. §227.7202, as applicable. In accordance with 48 C.F.R. §12.212 or 48 C.F.R. §227.7202-1 through 227.7202-4, as applicable, the Commercial Computer Software and Commercial Computer Software Documentation are being licensed to U.S. Government end users (a) only as Commercial Items and (b) with only those rights as are granted to all other end users pursuant to the terms and conditions herein.</p>

<h2>Export Compliance</h2>

<p>You may not export or re-export the Application except as authorized by United States law and the laws of the jurisdiction in which the Application was obtained.</p>
	
<p>In particular, but without limitation, the Application may not be exported or re-exported (a) into or to a nation or a resident of any U.S. embargoed countries or (b) to anyone on the U.S. Treasury Department's list of Specially Designated Nationals or the U.S. Department of Commerce Denied Person's List or Entity List.</p>

<p>By installing or using any component of the Application, you represent and warrant that you are not located in, under control of, or a national or resident of any such country or on any such list.</p>

<h2>Amendments to this Agreement</h2>

<p>Augmented Perception Corporation reserves the right, at its sole discretion, to modify or replace this Agreement at any time. If a revision is material we will provide at least 30 days' notice prior to any new terms taking effect. What constitutes a material change will be determined at our sole discretion.</p>

<p>By continuing to access or use our Application after any revisions become effective, you agree to be bound by the revised terms. If you do not agree to the new terms, you are no longer authorized to use the Application.</p>

<h2>Governing Law</h2>

<p>The laws of Delaware, United States, excluding its conflicts of law rules, shall govern this Agreement and your use of the Application. Your use of the Application may also be subject to other local, state, national, or international laws.</p>

<p>This Agreement shall not be governed by the United Nations Convention on Contracts for the International Sale of Good.</p>

<h2>Contact Information</h2>

<p>If you have any questions about this Agreement, please contact us.</p>

<h2>Entire Agreement</h2>

<p>The Agreement constitutes the entire agreement between you and Augmented Perception Corporation regarding your use of the Application and supersedes all prior and contemporaneous written or oral agreements between you and Augmented Perception Corporation.</p>

<p>You may be subject to additional terms and conditions that apply when you use or purchase other Augmented Perception Corporation's services, which Augmented Perception Corporation will provide to you at the time of such use or purchase.</p>
";

            webPrivacy.DocumentText =
@"
<h2>Privacy Policy</h2>


<p>Effective date: May 25, 2019</p>


<p>Augmented Perception Corporation (""us"", ""we"", or ""our"") operates the https://www.xrmonitors.com website (hereinafter referred to as the ""Service"").</p>

<p>This page informs you of our policies regarding the collection, use and disclosure of personal data when you use our Service and the choices you have associated with that data.</p>

<p>We use your data to provide and improve the Service. By using the Service, you agree to the collection and use of information in accordance with this policy. Unless otherwise defined in this Privacy Policy, the terms used in this Privacy Policy have the same meanings as in our Terms and Conditions, accessible from <a href=""https://www.xrmonitors.com/terms"">https://www.xrmonitors.com/terms</a></p>

<h2>Definitions</h2>
<ul>
    <li>
        <p><strong>Service</strong></p>
                <p>Service is the https://www.xrmonitors.com website operated by Augmented Perception Corporation</p>
            </li>
    <li>
        <p><strong>Personal Data</strong></p>
        <p>Personal Data means data about a living individual who can be identified from those data (or from those and other information either in our possession or likely to come into our possession).</p>
    </li>
    <li>
        <p><strong>Usage Data</strong></p>
        <p>Usage Data is data collected automatically either generated by the use of the Service or from the Service infrastructure itself (for example, the duration of a page visit).</p>
    </li>
    <li>
        <p><strong>Cookies</strong></p>
        <p>Cookies are small files stored on your device (computer or mobile device).</p>
    </li>
        <li>
        <p><strong>Data Controller</strong></p>
        <p>Data Controller means the natural or legal person who (either alone or jointly or in common with other persons) determines the purposes for which and the manner in which any personal information are, or are to be, processed.</p>
        <p>For the purpose of this Privacy Policy, we are a Data Controller of your Personal Data.</p>
    </li>
    <li>
        <p><strong>Data Processors (or Service Providers)</strong></p>
        <p>Data Processor (or Service Provider) means any natural or legal person who processes the data on behalf of the Data Controller.</p>
        <p>We may use the services of various Service Providers in order to process your data more effectively.</p>
    </li>
    <li>
        <p><strong>Data Subject (or User)</strong></p>
        <p>Data Subject is any living individual who is using our Service and is the subject of Personal Data.</p>
    </li>
    </ul>


<h2>Information Collection and Use</h2>
<p>We collect several different types of information for various purposes to provide and improve our Service to you.</p>

<h3>Types of Data Collected</h3>

<h4>Personal Data</h4>
<p>While using our Service, we may ask you to provide us with certain personally identifiable information that can be used to contact or identify you (""Personal Data""). Personally identifiable information may include, but is not limited to:</p>

<ul>
    <li>Email address</li>    <li>First name and last name</li>        <li>Address, State, Province, ZIP/Postal code, City</li>    <li>Cookies and Usage Data</li>
</ul>

<p>We may use your Personal Data to contact you with newsletters, marketing or promotional materials and other information that may be of interest to you. You may opt out of receiving any, or all, of these communications from us by following the unsubscribe link or the instructions provided in any email we send.</p>

<h4>Usage Data</h4>

<p>We may also collect information on how the Service is accessed and used (""Usage Data""). This Usage Data may include information such as your computer's Internet Protocol address (e.g. IP address), browser type, browser version, the pages of our Service that you visit, the time and date of your visit, the time spent on those pages, unique device identifiers and other diagnostic data.</p>


<h4>Tracking &amp; Cookies Data</h4>
<p>We use cookies and similar tracking technologies to track the activity on our Service and we hold certain information.</p>
<p>Cookies are files with a small amount of data which may include an anonymous unique identifier. Cookies are sent to your browser from a website and stored on your device. Other tracking technologies are also used such as beacons, tags and scripts to collect and track information and to improve and analyse our Service.</p>
<p>You can instruct your browser to refuse all cookies or to indicate when a cookie is being sent. However, if you do not accept cookies, you may not be able to use some portions of our Service.</p>
<p>Examples of Cookies we use:</p>
<ul>
    <li><strong>Session Cookies.</strong> We use Session Cookies to operate our Service.</li>
    <li><strong>Preference Cookies.</strong> We use Preference Cookies to remember your preferences and various settings.</li>
    <li><strong>Security Cookies.</strong> We use Security Cookies for security purposes.</li>
    <li><strong>Advertising Cookies.</strong> Advertising Cookies are used to serve you with advertisements that may be relevant to you and your interests.</li></ul>

<h2>Use of Data</h2> 
<p>Augmented Perception Corporation uses the collected data for various purposes:</p>    
<ul>
    <li>To provide and maintain our Service</li>
    <li>To notify you about changes to our Service</li>
    <li>To allow you to participate in interactive features of our Service when you choose to do so</li>
    <li>To provide customer support</li>
    <li>To gather analysis or valuable information so that we can improve our Service</li>
    <li>To monitor the usage of our Service</li>
    <li>To detect, prevent and address technical issues</li>
    <li>To provide you with news, special offers and general information about other goods, services and events which we offer that are similar to those that you have already purchased or enquired about unless you have opted not to receive such information</li></ul>

   
<h2>Legal Basis for Processing Personal Data under the General Data Protection Regulation (GDPR)</h2>
<p>If you are from the European Economic Area (EEA), Augmented Perception Corporation legal basis for collecting and using the personal information described in this Privacy Policy depends on the Personal Data we collect and the specific context in which we collect it.</p>
<p>Augmented Perception Corporation may process your Personal Data because:</p>
<ul>
    <li>We need to perform a contract with you</li>
    <li>You have given us permission to do so</li>
    <li>The processing is in our legitimate interests and it is not overridden by your rights</li>
    <li>For payment processing purposes</li>    <li>To comply with the law</li>
</ul>

    
<h2>Retention of Data</h2>    
<p>Augmented Perception Corporation will retain your Personal Data only for as long as is necessary for the purposes set out in this Privacy Policy. We will retain and use your Personal Data to the extent necessary to comply with our legal obligations (for example, if we are required to retain your data to comply with applicable laws), resolve disputes and enforce our legal agreements and policies.</p>
<p>Augmented Perception Corporation will also retain Usage Data for internal analysis purposes. Usage Data is generally retained for a shorter period of time, except when this data is used to strengthen the security or to improve the functionality of our Service, or we are legally obligated to retain this data for longer periods.</p>    

<h2>Transfer of Data</h2>
<p>Your information, including Personal Data, may be transferred to - and maintained on - computers located outside of your state, province, country or other governmental jurisdiction where the data protection laws may differ from those of your jurisdiction.</p>
<p>If you are located outside United States and choose to provide information to us, please note that we transfer the data, including Personal Data, to United States and process it there.</p>
<p>Your consent to this Privacy Policy followed by your submission of such information represents your agreement to that transfer.</p>
<p>Augmented Perception Corporation will take all the steps reasonably necessary to ensure that your data is treated securely and in accordance with this Privacy Policy and no transfer of your Personal Data will take place to an organisation or a country unless there are adequate controls in place including the security of your data and other personal information.</p>

<h2>Disclosure of Data</h2>
<h3>Business Transaction</h3>
<p>If Augmented Perception Corporation is involved in a merger, acquisition or asset sale, your Personal Data may be transferred. We will provide notice before your Personal Data is transferred and becomes subject to a different Privacy Policy.</p>    

<h3>Disclosure for Law Enforcement</h3>
<p>Under certain circumstances, Augmented Perception Corporation may be required to disclose your Personal Data if required to do so by law or in response to valid requests by public authorities (e.g. a court or a government agency).</p>

<h3>Legal Requirements</h3>
<p>Augmented Perception Corporation may disclose your Personal Data in the good faith belief that such action is necessary to:</p>
<ul>
    <li>To comply with a legal obligation</li>
    <li>To protect and defend the rights or property of Augmented Perception Corporation</li>
    <li>To prevent or investigate possible wrongdoing in connection with the Service</li>
    <li>To protect the personal safety of users of the Service or the public</li>
    <li>To protect against legal liability</li>
</ul>

<h2>Security of Data</h2>
<p>The security of your data is important to us but remember that no method of transmission over the Internet or method of electronic storage is 100% secure. While we strive to use commercially acceptable means to protect your Personal Data, we cannot guarantee its absolute security.</p>

<h2>Our Policy on ""Do Not Track"" Signals under the California Online Protection Act (CalOPPA)</h2>    
<p>We do not support Do Not Track (""DNT""). Do Not Track is a preference you can set in your web browser to inform websites that you do not want to be tracked.</p>
<p>You can enable or disable Do Not Track by visiting the Preferences or Settings page of your web browser.</p>

<h2>Your Data Protection Rights under the General Data Protection Regulation (GDPR)</h2>
<p>If you are a resident of the European Economic Area (EEA), you have certain data protection rights. Augmented Perception Corporation aims to take reasonable steps to allow you to correct, amend, delete or limit the use of your Personal Data.</p>
<p>If you wish to be informed about what Personal Data we hold about you and if you want it to be removed from our systems, please contact us.</p>
<p>In certain circumstances, you have the following data protection rights:</p>
<ul>
    <li>
        <p><strong>The right to access, update or delete the information we have on you.</strong> Whenever made possible, you can access, update or request deletion of your Personal Data directly within your account settings section. If you are unable to perform these actions yourself, please contact us to assist you.</p>
    </li>
    <li>
        <p><strong>The right of rectification.</strong> You have the right to have your information rectified if that information is inaccurate or incomplete.</p>
    </li> 
    <li>
        <p><strong>The right to object.</strong> You have the right to object to our processing of your Personal Data.</p>
    </li>
    <li>
        <p><strong>The right of restriction.</strong> You have the right to request that we restrict the processing of your personal information.</p>
    </li>
    <li>
        <p><strong>The right to data portability.</strong> You have the right to be provided with a copy of the information we have on you in a structured, machine-readable and commonly used format.</p>
    </li>
    <li>
        <p><strong>The right to withdraw consent.</strong> You also have the right to withdraw your consent at any time where Augmented Perception Corporation relied on your consent to process your personal information.</p>
    </li>
</ul>
<p>Please note that we may ask you to verify your identity before responding to such requests.</p>

<p>You have the right to complain to a Data Protection Authority about our collection and use of your Personal Data. For more information, please contact your local data protection authority in the European Economic Area (EEA).</p>

<h2>Service Providers</h2>
<p>We may employ third party companies and individuals to facilitate our Service (""Service Providers""), provide the Service on our behalf, perform Service-related services or assist us in analysing how our Service is used.</p>
<p>These third parties have access to your Personal Data only to perform these tasks on our behalf and are obligated not to disclose or use it for any other purpose.</p>

<h3>Analytics</h3>
<p>We may use third-party Service Providers to monitor and analyse the use of our Service.</p>    
<ul>
        <li>
        <p><strong>Google Analytics</strong></p>
        <p>Google Analytics is a web analytics service offered by Google that tracks and reports website traffic. Google uses the data collected to track and monitor the use of our Service. This data is shared with other Google services. Google may use the collected data to contextualise and personalise the ads of its own advertising network.</p>
        <p>You can opt-out of having made your activity on the Service available to Google Analytics by installing the Google Analytics opt-out browser add-on. The add-on prevents the Google Analytics JavaScript (ga.js, analytics.js and dc.js) from sharing information with Google Analytics about visits activity.</p>                <p>For more information on the privacy practices of Google, please visit the Google Privacy &amp; Terms web page: <a href=""https://policies.google.com/privacy?hl=en"">https://policies.google.com/privacy?hl=en</a></p>
    </li>
                                </ul>

<h3>Advertising</h3>
<p>We may use third-party Service Providers to show advertisements to you to help support and maintain our Service.</p>
<ul>
        <li>
        <p><strong>Google AdSense &amp; DoubleClick Cookie</strong></p>
        <p>Google, as a third party vendor, uses cookies to serve ads on our Service. Google's use of the DoubleClick cookie enables it and its partners to serve ads to our users based on their visit to our Service or other websites on the Internet.</p>
        <p>You may opt out of the use of the DoubleClick Cookie for interest-based advertising by visiting the Google Ads Settings web page: <a href=""http://www.google.com/ads/preferences/"">http://www.google.com/ads/preferences/</a></p>
    </li>
                                                </ul>
   


<h3>Payments</h3>
<p>We may provide paid products and/or services within the Service. In that case, we use third-party services for payment processing (e.g. payment processors).</p>
<p>We will not store or collect your payment card details. That information is provided directly to our third-party payment processors whose use of your personal information is governed by their Privacy Policy. These payment processors adhere to the standards set by PCI-DSS as managed by the PCI Security Standards Council, which is a joint effort of brands like Visa, MasterCard, American
Express and Discover. PCI-DSS requirements help ensure the secure handling of payment information.</p>
<p>The payment processors we work with are:</p>
<ul>
                <li>
        <p><strong>Stripe</strong></p>
        <p>Their Privacy Policy can be viewed at <a href=""https://stripe.com/us/privacy"">https://stripe.com/us/privacy</a></p>
    </li>
                        <li>
        <p><strong>PayPal / Braintree</strong></p>
        <p>Their Privacy Policy can be viewed at <a href=""https://www.paypal.com/webapps/mpp/ua/privacy-full"">https://www.paypal.com/webapps/mpp/ua/privacy-full</a></p>
    </li>
                <li>
        <p><strong>Authorize.net</strong></p>
        <p>Their Privacy Policy can be viewed at <a href=""https://www.authorize.net/company/privacy/"">https://www.authorize.net/company/privacy/</a></p>
    </li>
                                        </ul>


<h2>Links to Other Sites</h2>
<p>Our Service may contain links to other sites that are not operated by us. If you click a third party link, you will be directed to that third party's site. We strongly advise you to review the Privacy Policy of every site you visit.</p>
<p>We have no control over and assume no responsibility for the content, privacy policies or practices of any third party sites or services.</p>


<h2>Children's Privacy</h2>
<p>Our Service does not address anyone under the age of 18 (""Children"").</p>
<p>We do not knowingly collect personally identifiable information from anyone under the age of 18. If you are a parent or guardian and you are aware that your Child has provided us with Personal Data, please contact us. If we become aware that we have collected Personal Data from children without verification of parental consent, we take steps to remove that information from our servers.</p>


<h2>Changes to This Privacy Policy</h2>
<p>We may update our Privacy Policy from time to time. We will notify you of any changes by posting the new Privacy Policy on this page.</p>
<p>We will let you know via email and/or a prominent notice on our Service, prior to the change becoming effective and update the ""effective date"" at the top of this Privacy Policy.</p>
<p>You are advised to review this Privacy Policy periodically for any changes. Changes to this Privacy Policy are effective when they are posted on this page.</p>


<h2>Contact Us</h2>
<p>If you have any questions about this Privacy Policy, please contact us:</p>
<ul>
        <li>By email: support@xrmonitors.com</li>
</ul>
";
        }

        private void ButtonExit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void ButtonAccept_Click(object sender, EventArgs e)
        {
            Program.setup_form.install_panel.Show();
            this.Hide();
        }
    }
}
